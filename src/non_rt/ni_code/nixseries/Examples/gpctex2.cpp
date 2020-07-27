/*
 * gpctex2.cpp
 *   Single-point edge counting with start trigger and hardware controlled direction
 *
 * gpctex2 counts TTL edges and transfers data to the host by reading directly
 * from the save register. After configuring the counter's gating parameters,
 * gpctex2 arms the counter and waits for an arm-start trigger. Once started,
 * data is read and printed until the measurement is complete and gpctex2
 * disarms the counter. Finally, gpctex2 restores the hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : edge counting
 * Channel
 *   Channels        : ctr0 (*), ctr1, ctr2 or ctr3
 *   Scaling         : none
 *   Input terminal  : PFI0
 *   Input polarity  : rising edge (*) or falling edge
 * ! Direction term  : PFI1
 * Timing
 *   Sample mode     : single-point
 *   Timing mode     : software-timed
 *   Clock source    : implicit
 * Trigger
 * ! Start trigger   : PFI2 (digital rising edge) as arm-start
 *   Reference trig  : not supported
 *   Pause trigger   : none
 * Read Buffer
 *   Data transfer   : programmed IO from save register
 * Behavior
 *   Timeout         : 10 seconds
 *
 * External Connections
 *   PFI0            : TTL input signal
 *   PFI1            : TTL up/down signal
 *   PFI2            : TTL arm-start trigger
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

   //
   // Feature Parameters (modify these to exercise the example)
   //

   // Channel parameters
   const u32 counterNumber = 0;
   const nCounter::tGi_Source_Select_t source = nCounter::kSrc_PFI0;
   const nCounter::tGi_Polarity_t sourcePolarity = nCounter::kActiveHigh;
   const nCounter::tGi_B_Select_t directionSource = nCounter::kB_PFI1;

   // Trigger parameters
   const nCounter::tGi_HW_Arm_Select_t hwArmSource = nCounter::kHwArm_PFI2;
   const nCounter::tGi_Polarity_t hwArmPolariy = nCounter::kActiveHigh;

   // Buffer parameters
   const u32 sampsPerChan = 20;

   // Behavior parameters
   const f64 timeout = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Device parameters
   tCounter* counter = NULL;

   // Buffer parameters
   u32 samplesRead = 0;
   u32 lastValue = 0; // Cache the last count from the counter
   u32 n = 0;  // Number of samples counter
   const u32 samplesPerRead = 1;

   std::vector<u32> rawData(samplesPerRead, 0);

   // Behavior parameters
   f64 elapsedTime = 0; // How long has the measurement been running?
   clock_t start;
   const f64 rlpTimeout = 5; // Wait 5 seconds for a register operation before breaking
   clock_t rlpStart;
   f64 rlpElapsedTime;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tAddressSpace bar0;
   bar0 = bus->createAddressSpace(kPCI_BAR0);

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
   counter->Gi_Mode_Register.setGi_Loading_On_Gate(nCounter::kNoCounterReloadOnGate, &status);
   counter->Gi_Mode_Register.setGi_ForceSourceEqualToTimebase(kFalse, &status);
   counter->Gi_Mode_Register.setGi_Loading_On_TC(nCounter::kRolloverOnTC, &status);
   counter->Gi_Mode_Register.setGi_Counting_Once(nCounter::kNoHardwareDisarm, &status);
   // Gi_Output_Mode doesn't matter
   counter->Gi_Mode_Register.setGi_Load_Source_Select(nCounter::kLoad_From_Register_A, &status);
   // Gi_Stop_Mode doesn't matter
   // Gi_Trigger_Mode_For_Edge_Gate doesn't matter
   // Gi_Gate_On_Both_Edges doesn't matter
   counter->Gi_Mode_Register.setGi_Gating_Mode(nCounter::kGateDisabled, &status);
   counter->Gi_Mode_Register.flush(&status);

   counter->Gi_Mode2_Register.setGi_Up_Down(nCounter::kGi_B_High_Up, &status);
   counter->Gi_Mode2_Register.setGi_Bank_Switch_Enable(nCounter::kDisabled_If_Armed_Else_Write_To_X, &status);
   counter->Gi_Mode2_Register.setGi_Bank_Switch_Mode(nCounter::kSoftware, &status);
   // Gi_WriteOnSwitchRequest doesn't matter
   // Gi_StopOnError doesn't matter
   // Gi_CtrOutFifoRegenerationEn doesn't matter
   // Gi_HwArmSyncMode doesn't matter
   counter->Gi_Mode2_Register.flush(&status);

   // Gi_Prescale_Div_2 doesn't matter
   counter->Gi_Counting_Mode_Register.setGi_Prescale(kFalse, &status);
   counter->Gi_Counting_Mode_Register.setGi_HW_Arm_Select(hwArmSource, &status);
   counter->Gi_Counting_Mode_Register.setGi_HW_Arm_Enable(kTrue, &status);
   // Gi_Index_Phase doesn't matter
   counter->Gi_Counting_Mode_Register.setGi_Index_Mode(nCounter::kIndexModeCleared, &status);
   counter->Gi_Counting_Mode_Register.setGi_HW_Arm_Polarity(hwArmPolariy, &status);
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

   // Gi_Second_Gate_Polarity doesn't matter
   // Gi_Second_Gate_Select doesn't matter
   counter->Gi_Second_Gate_Register.setGi_Second_Gate_Mode(nCounter::kDisabledSecondGate, &status);
   counter->Gi_Second_Gate_Register.flush(&status);

   counter->Gi_Input_Select_Register.setGi_Source_Polarity(sourcePolarity, &status);
   // Gi_Output_Polarity doesn't matter
   // Gi_Gate_Select_Load_Source doesn't matter
   // Gi_Gate_Select doesn't matter
   counter->Gi_Input_Select_Register.setGi_Source_Select(source, &status);
   // Gi_Gate_Polarity doesn't matter
   counter->Gi_Input_Select_Register.flush(&status);

   // Gi_Z_Select doesn't matter
   counter->Gi_ABZ_Select_Register.setGi_B_Select(directionSource, &status);
   // Gi_A_Select doesn't matter
   counter->Gi_ABZ_Select_Register.flush(&status);

   //
   // Program the FIFO
   //

   // Gi_DoneNotificationEnable doesn't matter
   // Gi_WrFifoEnable doesn't matter
   // Gi_WaitForFirstEventOnGate doesn't matter
   // Gi_DMA_Reset doesn't matter
   // Gi_DMA_Write doesn't matter
   counter->Gi_DMA_Config_Register.setGi_DMA_Enable(kFalse, &status);
   counter->Gi_DMA_Config_Register.flush(&status);

   /*********************************************************************\
   |
   |   Start the edge counting measurement
   |
   \*********************************************************************/

   //
   // Prime the counter
   //

   counter->Gi_Load_A_Register.writeRegister(0, &status);
   counter->Gi_Command_Register.writeGi_Load(kTrue, &status);

   //
   // Arm the counter
   //

   counter->Gi_Command_Register.writeGi_Arm(kTrue, &status);

   // The counter arms via an external signal, so don't wait for it to arm.

   //
   // Wait for the start trigger
   //

   start = clock();
   while (!counter->Gi_Status_Register.readGi_Counting_St(&status))
   {
      printf("Waiting %.2f seconds for start trigger...\r", timeout-elapsedTime);
      if (elapsedTime > timeout)
      {
         printf("\n");
         printf("Error: Trigger timeout.\n");
         status.setCode(kStatusRLPTimeout);
         return;
      }
      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
   }
   printf("\n");

   /*********************************************************************\
   |
   |   Read and print data
   |
   \*********************************************************************/

   nNISTC3::nGPCTDataHelper::printHeader("Count Edges", "   Ticks");

   elapsedTime = 0;
   start = clock();
   for (n=0; n<sampsPerChan; ++n)
   {
      // Wait for a new value from the counter
      while ((rawData[0] = counter->Gi_Save_Register.readRegister(&status)) == lastValue)
      {
         if (elapsedTime > timeout) break;
         elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
      }

      if (elapsedTime > timeout)
      {
         printf("\n");
         printf("Error: Measurement did not complete within %.2f second timeout.\n", timeout);
         break;
      }

      nNISTC3::nGPCTDataHelper::printData(rawData, samplesPerRead, 1);
      lastValue = rawData[0];
      ++samplesRead;
   }
   printf("\n");

   /*********************************************************************\
   |
   |   Check for counter subsystem errors
   |
   \*********************************************************************/

   // The counter is not used in a way that generates errors

   /*********************************************************************\
   |
   |   Stop the edge counting measurement
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

   printf("Finished finite edge counting measurement.\n");
   printf("Read %u samples.\n", samplesRead);

   //
   // Restore the state of the device
   //

   // Nothing else to do: the helpers' destructors safely unwind device state.
}
