/*
 * gpctex1.cpp
 *   Single-point on-demand pulse train measurement
 *
 * gpctex1 measures pulse train characteristics and transfers data to the host
 * by reading directly from the save register or FIFO. After configuring the
 * counter's gating parameters, gpctex1 arms the counter via software. Once
 * started, data is read and printed until the measurement is complete and
 * gpctex1 disarms the counter. Finally, gpctex1 restores the hardware's
 * previous state.
 *
 * gpctex1 measures the following pulse train characteristics:
 * Digital Edges (count)
 *   In this mode, the signal to count is routed to the Counter's source,
 *   and gating is disabled.
 * Period (ticks and time)
 *   In this mode, a timebase is routed to the Counter's source, and the
 *   signal to measure is routed to the Counter's gate. One edge of the
 *   gate latches the current count into the FIFO and reloads the count.
 * Semi-Period (ticks and time)
 *   In this mode, a timebase is routed to the Counter's source, and the
 *   signal to measure is routed to the Counter's gate. Both edges of
 *   the gate latch the current count into the FIFO and reload the count.
 * 2-Edge Separation (ticks and time)
 *   In this mode, a timebase is routed to the Counter's source, the
 *   first edge is routed to the Counter's second gate, and the second edge
 *   is routed to the Counter's gate. These two signals combine to become
 *   a level-sensitive gate.  When the gate asserts the Counter begins
 *   counting, and when it deasserts it latches the current count into the
 *   FIFO and reloads the count.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : pulse train measurement
 * Channel
 * ! Channels        : ctr0 (*), ctr1, ctr2 or ctr3
 * ! Scaling         : seconds (*) or raw counts
 * ! Input terminal  : PFI0
 * ! Input polarity  : rising edge (*) or falling edge
 * Timing
 * ! Sample mode     : single-point
 * ! Timing mode     : software-timed for edge counting
 * !                   implicit for period, semi-period, 2-edge separation
 * ! Clock source    : implicit for edge counting
 * !                   on-board oscillator for period, semi-period, 2-edge sep
 * Trigger
 * ! Start trigger   : software
 *   Reference trig  : not supported
 *   Pause trigger   : none
 * Read Buffer
 *   Data transfer   : programmed IO from
 * !                     save register for edge counting
 * !                     FIFO for period, semi-period, 2-edge separation
 * Behavior
 *   Timeout         : 10 seconds
 *
 * External Connections
 *   PFI0            : TTL input signal
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include <stdio.h>
#include <time.h>
#include <vector>

// OS Interface
#include "osiBus.h"

// Chip Objects
#include "tXSeries.h"

// Chip Object Helpers
#include "counterResetHelper.h"
#include "dataHelper.h"
#include "devices.h"
#include "simultaneousInit.h"

void test(iBus* bus)
{
   /*********************************************************************\
   |
   |   Example parameters
   |
   \*********************************************************************/

   enum tOperation
   {
      kCountEdges = 0,
      kPeriod,
      kSemiPeriod,
      kTwoEdgeSeparation
   };

   //
   // Feature Parameters (modify these to exercise the example)
   //

   // Channel parameters
   const u32 counterNumber = 0;
   tBoolean printTime = kTrue;
   tOperation operation = kPeriod;

   // Count Edges parameters
   const nCounter::tGi_Source_Select_t countEdgesSource = nCounter::kSrc_PFI0;
   const nCounter::tGi_Polarity_t countEdgesPolarity = nCounter::kActiveHigh;

   // Timebase parameters - this is the timebase for everything but Count Edges
   const nCounter::tGi_Source_Select_t timebaseSource = nCounter::kSrc_TB3;
   const nCounter::tGi_Polarity_t timebasePolarity = nCounter::kActiveHigh;
   const f32 timebaseRate = 100000000.0;

   // Gate parameters
   //  * Count Edges - unused
   //  * Period - Signal to Measure
   //  * Semi Period - Signal to Measure
   //  * Two Edge Separation - Second Edge
   const nCounter::tGi_Gate_Select_t gateSource = nCounter::kGate_PFI0;
   const nCounter::tGi_Polarity_t gatePolarity = nCounter::kActiveHigh;

   // Second gate parameters
   //  * Count Edges - unused
   //  * Period - unused
   //  * Semi Period - unused
   //  * Two Edge Separation - First Edge
   const nCounter::tGi_Second_Gate_Select_t gate2Source = nCounter::kGate2_PFI0;
   const nCounter::tGi_Polarity_t gate2Polarity = nCounter::kActiveLow;

   // Buffer parameters
   const u32 sampsPerChan = 20;

   // Behavior parameters
   const f64 timeout = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Device parameters
   tCounter* counter = NULL;

   // Channel parameters
   nNISTC3::nGPCTDataHelper::tDigWaveformScaler_t scaler;
   scaler.timebaseRate = timebaseRate;

   // Buffer parameters
   u32 samplesRead = 0;
   u32 lastValue = 0; // Cache the last count from the counter
   u32 n = 0;  // Number of samples counter
   const u32 samplesPerRead = 1;

   std::vector<u32> rawData(samplesPerRead, 0);
   std::vector<f32> scaledData(samplesPerRead, 0);

   // Behavior parameters
   f64 elapsedTime = 0; // How long has the measurement been running?
   clock_t start;
   const f64 rlpTimeout = 5; // Wait 5 seconds for a register operation before breaking
   clock_t rlpStart;
   f64 rlpElapsedTime;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tBoolean gpctErrored = kFalse; // Did the counter subsystem have an error?
   u32 DRQ_Error = 0;
   tAddressSpace bar0;
   bar0 = bus->createAddressSpace(kPCI_BAR0);

   char* measurementLabel = NULL;
   char* unitsLabel = NULL;
   char* ticksLabel = "   Ticks";
   char* timeLabel = "    Seconds";

   switch (operation)
   {
   case kCountEdges:
      printTime = kFalse;
      measurementLabel = "Count Edges";
      break;
   case kPeriod:
      measurementLabel = "Period";
      break;
   case kSemiPeriod:
      measurementLabel = "Semi Period";
      break;
   case kTwoEdgeSeparation:
      measurementLabel = "Two-Edge Sep";
      break;
   }
   unitsLabel = printTime ? timeLabel : ticksLabel;

   /*********************************************************************\
   |
   |   Initialize the measurement
   |
   \*********************************************************************/

   //
   // Open, detect, and initialize the X Series device
   //

   tXSeries device(bar0, &status);

   const nNISTC3::tDeviceInfo* deviceInfo = nNISTC3::getDeviceInfo(device, status);
   if (status.isFatal())
   {
      printf("Error: Cannot identify device (%d).\n", status.statusCode);
      return;
   }

   if (deviceInfo->isSimultaneous) nNISTC3::initializeSimultaneousXSeries(device, status);

   switch (counterNumber)
   {
   case 0: counter = &device.Counter0; break;
   case 1: counter = &device.Counter1; break;
   case 2: counter = &device.Counter2; break;
   case 3: counter = &device.Counter3; break;
   default:
      printf("Error: Invalid counter.\n");
      status.setCode(kStatusBadSelector);
      return;
   }

   //
   // Create subsystem helpers
   //

   nNISTC3::counterResetHelper counterResetHelper(
      *counter,                     // Counter used for this example
      kFalse,                       // preserveOutputState
      status);

   //
   // Validate the Feature Parameters
   //

   // Nothing else to validate

   /*********************************************************************\
   |
   |   Program peripheral subsystems
   |
   \*********************************************************************/

   // Set all PFI lines to be input
   device.Triggers.PFI_Direction_Register.writeRegister(0x0, &status);

   /*********************************************************************\
   |
   |   Program the counter subsystem
   |
   \*********************************************************************/

   //
   // Reset the counter subsystem
   //

   counterResetHelper.reset( /*initialReset*/ kTrue, status);

   //
   // Program the counter
   //

   counter->Gi_Mode_Register.setGi_Reload_Source_Switching(nCounter::kUseSameLoadRegister, &status);
   if (operation == kCountEdges)
   {
      counter->Gi_Mode_Register.setGi_Loading_On_Gate(nCounter::kNoCounterReloadOnGate, &status);
   }
   else
   {
      counter->Gi_Mode_Register.setGi_Loading_On_Gate(nCounter::kReloadOnStopGate, &status);
   }
   counter->Gi_Mode_Register.setGi_ForceSourceEqualToTimebase(kFalse, &status);
   counter->Gi_Mode_Register.setGi_Loading_On_TC(nCounter::kRolloverOnTC, &status);
   counter->Gi_Mode_Register.setGi_Counting_Once(nCounter::kNoHardwareDisarm, &status);
   // Gi_Output_Mode doesn't matter
   counter->Gi_Mode_Register.setGi_Load_Source_Select(nCounter::kLoad_From_Register_A, &status);
   if (operation != kCountEdges)
   {
      counter->Gi_Mode_Register.setGi_Stop_Mode(nCounter::kStopOnGateCondition, &status);
      switch (operation)
      {
      case kPeriod:
         counter->Gi_Mode_Register.setGi_Gate_On_Both_Edges(nCounter::kDisabled, &status);
         counter->Gi_Mode_Register.setGi_Gating_Mode(nCounter::kAssertingEdgeGating, &status);
         counter->Gi_Mode_Register.setGi_Trigger_Mode_For_Edge_Gate(nCounter::kGateLoads, &status);
         break;
      case kSemiPeriod:
         counter->Gi_Mode_Register.setGi_Gate_On_Both_Edges(nCounter::kEnabled, &status);
         counter->Gi_Mode_Register.setGi_Gating_Mode(nCounter::kDeassertingEdgeGating, &status);
         counter->Gi_Mode_Register.setGi_Trigger_Mode_For_Edge_Gate(nCounter::kGateLoads, &status);
         break;
      case kTwoEdgeSeparation:
         counter->Gi_Mode_Register.setGi_Gate_On_Both_Edges(nCounter::kDisabled, &status);
         counter->Gi_Mode_Register.setGi_Gating_Mode(nCounter::kLevelGating, &status);
         counter->Gi_Mode_Register.setGi_Trigger_Mode_For_Edge_Gate(nCounter::kGateEdgeStarts, &status);
         break;
      }
   }
   else
   {
      counter->Gi_Mode_Register.setGi_Gating_Mode(nCounter::kGateDisabled, &status);
   }
   counter->Gi_Mode_Register.flush(&status);

   counter->Gi_Mode2_Register.setGi_Up_Down(nCounter::kCountUp, &status);
   counter->Gi_Mode2_Register.setGi_Bank_Switch_Enable(nCounter::kDisabled_If_Armed_Else_Write_To_X, &status);
   counter->Gi_Mode2_Register.setGi_Bank_Switch_Mode(nCounter::kSoftware, &status);
   // Gi_WriteOnSwitchRequest doesn't matter
   // Gi_StopOnError doesn't matter
   // Gi_CtrOutFifoRegenerationEn doesn't matter
   // Gi_HwArmSyncMode doesn't matter
   counter->Gi_Mode2_Register.flush(&status);

   // Gi_Prescale_Div_2 doesn't matter
   counter->Gi_Counting_Mode_Register.setGi_Prescale(kFalse, &status);
   // Gi_HW_Arm_Select doesn't matter
   counter->Gi_Counting_Mode_Register.setGi_HW_Arm_Enable(kFalse, &status);
   // Gi_Index_Phase doesn't matter
   counter->Gi_Counting_Mode_Register.setGi_Index_Mode(nCounter::kIndexModeCleared, &status);
   // Gi_HW_Arm_Polarity doesn't matter
   counter->Gi_Counting_Mode_Register.setGi_Counting_Mode(nCounter::kNormalCounting, &status);
   counter->Gi_Counting_Mode_Register.flush(&status);

   // Gi_SampleClockGateIndependent doesn't matter
   // Gi_SampleClockSampleMode doesn't matter
   // Gi_SampleClockPulse doesn't matter
   counter->Gi_SampleClockRegister.setGi_SampleClockMode(nCounter::kSC_Disabled, &status);
   // Gi_SampleClockLevelMode doesn't matter
   // Gi_SampleClockPolarity doesn't matter
   // Gi_SampleClockSelect doesn't matter
   counter->Gi_SampleClockRegister.flush(&status);

   counter->Gi_AuxCtrRegister.writeGi_AuxCtrMode(nCounter::kAux_Disabled, &status);

   // Gi_Autoincrement_Register doesn't matter

   if (operation != kTwoEdgeSeparation)
   {
      // Gi_Second_Gate_Polarity doesn't matter
      // Gi_Second_Gate_Select doesn't matter
      counter->Gi_Second_Gate_Register.setGi_Second_Gate_Mode(nCounter::kDisabledSecondGate, &status);
   }
   else
   {
      counter->Gi_Second_Gate_Register.setGi_Second_Gate_Polarity(gate2Polarity, &status);
      counter->Gi_Second_Gate_Register.setGi_Second_Gate_Select(gate2Source, &status);
      counter->Gi_Second_Gate_Register.setGi_Second_Gate_Mode(nCounter::kSecondGateAssertFirstDeassertsGate, &status);
   }
   counter->Gi_Second_Gate_Register.flush(&status);

   if (operation == kCountEdges)
   {
      counter->Gi_Input_Select_Register.setGi_Source_Polarity(countEdgesPolarity, &status);
      // Gi_Output_Polarity doesn't matter
      // Gi_Gate_Select_Load_Source doesn't matter
      // Gi_Gate_Select doesn't matter
      counter->Gi_Input_Select_Register.setGi_Source_Select(countEdgesSource, &status);
      // Gi_Gate_Polarity doesn't matter
   }
   else
   {
      counter->Gi_Input_Select_Register.setGi_Source_Polarity(timebasePolarity, &status);
      // Gi_Output_Polarity doesn't matter
      // Gi_Gate_Select_Load_Source doesn't matter
      counter->Gi_Input_Select_Register.setGi_Gate_Select(gateSource, &status);
      counter->Gi_Input_Select_Register.setGi_Source_Select(timebaseSource, &status);
      counter->Gi_Input_Select_Register.setGi_Gate_Polarity(gatePolarity, &status);
   }
   counter->Gi_Input_Select_Register.flush(&status);

   // Gi_ABZ_Select_Register doesn't matter

   //
   // Program the FIFO
   //

   if (operation == kCountEdges)
   {
      // Gi_DoneNotificationEnable doesn't matter
      // Gi_WrFifoEnable doesn't matter
      // Gi_WaitForFirstEventOnGate doesn't matter
      // Gi_DMA_Reset doesn't matter
      // Gi_DMA_Write doesn't matter
      counter->Gi_DMA_Config_Register.setGi_DMA_Enable(kFalse, &status);
   }
   else
   {
      counter->Gi_DMA_Config_Register.setGi_DoneNotificationEnable(kFalse, &status);
      counter->Gi_DMA_Config_Register.setGi_WrFifoEnable(kFalse, &status);
      if (operation != kTwoEdgeSeparation)
      {
         counter->Gi_DMA_Config_Register.setGi_WaitForFirstEventOnGate(1, &status);
      }
      else
      {
         counter->Gi_DMA_Config_Register.setGi_WaitForFirstEventOnGate(0, &status);
      }
      counter->Gi_DMA_Config_Register.setGi_DMA_Write(kFalse, &status);
      counter->Gi_DMA_Config_Register.setGi_DMA_Enable(kTrue, &status);
   }
   counter->Gi_DMA_Config_Register.flush(&status);

   /*********************************************************************\
   |
   |   Start the pulse train measurement
   |
   \*********************************************************************/

   //
   // Prime the counter
   //

   if (operation == kCountEdges || operation == kTwoEdgeSeparation)
   {
      counter->Gi_Load_A_Register.writeRegister(0, &status);
   }
   else
   {
      // If we're counting the Counter's timebase, we need to preload with 1, rather than 0.
      if ( counter->Gi_Input_Select_Register.getGi_Source_Select(&status) == nCounter::kSrc_TB3 ||
           counter->Gi_Mode_Register.getGi_ForceSourceEqualToTimebase(&status) == 1 )
      {
         counter->Gi_Load_A_Register.writeRegister(1, &status);
      }
      else
      {
         counter->Gi_Load_A_Register.writeRegister(0, &status);
      }
   }
   counter->Gi_Command_Register.writeGi_Load(kTrue, &status);

   //
   // Arm and start the counter
   //

   printf("Starting on-demand pulse train measurement.\n");
   counter->Gi_Command_Register.writeGi_Arm(kTrue, &status);

   // Wait for the counter to arm
   rlpElapsedTime = 0;
   rlpStart = clock();
   while (counter->Gi_Status_Register.readGi_Armed_St(&status) == nCounter::kNot_Armed)
   {
      // Spin on the Gi Armed state
      if (rlpElapsedTime > rlpTimeout)
      {
         printf("Error: Counter did not arm within timeout.\n");
         status.setCode(kStatusRLPTimeout);
         return;
      }
      rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
   }

   /*********************************************************************\
   |
   |   Read and print data
   |
   \*********************************************************************/

   nNISTC3::nGPCTDataHelper::printHeader(measurementLabel, unitsLabel);

   elapsedTime = 0;
   start = clock();
   for (n=0; n<sampsPerChan; ++n)
   {
      if (operation == kCountEdges)
      {
         // Wait for a new value from the counter
         while ((rawData[0] = counter->Gi_Save_Register.readRegister(&status)) == lastValue)
         {
            if (elapsedTime > timeout) break;
            elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
         }

         if (elapsedTime > timeout) break;
         lastValue = rawData[0];
         ++samplesRead;
      }
      else
      {
         // Check for counter subsystem errors
         counter->Gi_Status_Register.refresh(&status);
         DRQ_Error = counter->Gi_Status_Register.getGi_DRQ_Error(&status);

         if (DRQ_Error)
         {
            gpctErrored = kTrue;
            break;
         }

         // Wait for a new value from the counter
         while ((counter->Gi_FifoStatusRegister.readRegister(&status)) == 0)
         {
            if (elapsedTime > timeout) break;
            elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
         }

         if (elapsedTime > timeout) break;
         rawData[0] = counter->Gi_RdFifoRegister.readRegister(&status);
         ++samplesRead;
      }

      if (printTime)
      {
         nNISTC3::nGPCTDataHelper::scaleData(rawData, samplesPerRead, scaledData, samplesPerRead, scaler);
         nNISTC3::nGPCTDataHelper::printData(scaledData, samplesPerRead, 1);
      }
      else
      {
         nNISTC3::nGPCTDataHelper::printData(rawData, samplesPerRead, 1);
      }
   }
   printf("\n");

   /*********************************************************************\
   |
   |   Stop the pulse train measurement
   |
   \*********************************************************************/

   counter->Gi_Command_Register.writeGi_Disarm(kTrue, &status);

   // Wait for the counter to disarm
   rlpElapsedTime = 0;
   rlpStart = clock();
   while (counter->Gi_Status_Register.readGi_Armed_St(&status) == nCounter::kArmed)
   {
      // Spin on the Gi Armed state
      if (rlpElapsedTime > rlpTimeout)
      {
         printf("Error: Counter did not disarm within timeout.\n");
         status.setCode(kStatusRLPTimeout);
         return;
      }
      rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
   }

   /*********************************************************************\
   |
   |   Finalize the measurement
   |
   \*********************************************************************/

   //
   // Print run-time summary
   //

   if (DRQ_Error)
   {
      printf("Error: FIFO overflow.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (!gpctErrored)
   {
      printf("Finished on-demand pulse train measurement.\n");
      printf("Read %u samples.\n", samplesRead);
   }

   //
   // Restore the state of the device
   //

   // Nothing else to do: the helpers' destructors safely unwind device state.
}
