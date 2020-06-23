/*
 * gpctex7.cpp
 *   Single-point implicitly-timed pulse train output
 *
 * gpctex7 scales and generates a pulse train using implicit timing and
 * transfers data to the device by writing directly to the counter
 * registers. After configuring the counter's gating parameters, gpctex7
 * creates a simple pulse train to generate before sending a software start
 * trigger. The counter generates pulses, following the specified idle count
 * with the specified active count. After the generation is complete, gpctex7
 * disarms the counter. Finally, gpctex7 restores the hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : pulse train generation
 * Channel
 * ! Channels        : ctr0 (*), ctr1, ctr2 or ctr3
 *   Scaling         : time (*) or raw counts
 * ! Output terminal : PFI0
 * ! Output polarity : active low (*) or active high
 * ! Output state    : preserve on exit (*) or tristate
 * Timing
 * ! Sample mode     : single-point
 * ! Timing mode     : implicit
 * ! Clock source    : on-board oscillator (*) or external PFI line
 * ! Clock polarity  : rising edge (*) or falling edge
 * Trigger
 * ! Start trigger   : software
 *   Reference trig  : not supported
 *   Pause trigger   : none
 * Write Buffer
 * ! Data transfer   : programmed IO to registers
 * Behavior
 *   Runtime         : 10 seconds
 *
 * External Connections
 *   PFI0            : observe on an oscilloscope
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
#include "pfiRtsiResetHelper.h"
#include "simultaneousInit.h"

void test(iBus* bus, f32 dutyCycle, u32 frequency, f64 runTime)
{
   /*********************************************************************\
   |
   |   Example parameters
   |
   \*********************************************************************/

   enum tMode
   {
      kSinglePulse = 0,
      kFinitePulseTrain,
      kContinuousPulseTrain
   };

   //
   // Feature Parameters (modify these to exercise the example)
   //

   // Channel parameters
   const u32 counterNumber = 1;
   tBoolean printTime = kTrue;
   const nCounter::tGi_Polarity_t outputPolarity = nCounter::kActiveLow;
   tBoolean preserveOutputState = kTrue;
   tMode mode = kContinuousPulseTrain;
   u32 numberOfPulsesForFinite = 5;
   const f32 delayTime  = 3.0F;
   const f32 activeTime = dutyCycle / 100.0;
   const f32 idleTime   = 1 - (dutyCycle / 100.0);

   // Timing parameters
   const nCounter::tGi_Source_Select_t timebaseSource = nCounter::kSrc_TB3;
   const nCounter::tGi_Polarity_t timebasePolarity = nCounter::kActiveHigh;
   const f32 timebaseRate = 100000000.0 / static_cast<f32>(frequency);

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Device parameters
   tCounter* counter = NULL;

   // Channel parameters
   nNISTC3::nGPCTDataHelper::tPulseTrainScaler_t scaler;
   scaler.timebaseRate = timebaseRate;
   u32 generatedPulses = 0;
   u32 countsLeft = 0;

   // Buffer parameters
   const u32 totalNumberOfSamples = 4;

   std::vector<f32> timeSpecs(totalNumberOfSamples, 0);
   std::vector<u32> pulseSpecs(totalNumberOfSamples, 1); // The counter must count at least two counts

   // Behavior parameters
   f64 elapsedTime = 0; // How long has the generation been running?
   clock_t start;
   const f64 rlpTimeout = 5; // Wait 5 seconds for a register operation before breaking
   clock_t rlpStart;
   f64 rlpElapsedTime;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tAddressSpace bar0;
   bar0 = bus->createAddressSpace(kPCI_BAR0);

   char* generationLabel = NULL;
   switch (mode)
   {
   case kSinglePulse:          generationLabel = "Single Pulse"; break;
   case kFinitePulseTrain:     generationLabel = "Finite Train"; break;
   case kContinuousPulseTrain: generationLabel = "Cont Train";   break;
   }

   /*********************************************************************\
   |
   |   Initialize the generation
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

   nNISTC3::pfiRtsiResetHelper pfiRtsiResetHelper(
      device.Triggers,              // Trigger subsystem for access to PFI and RTSI lines
      preserveOutputState,          // preserveOutputState
      status);

   nNISTC3::counterResetHelper counterResetHelper(
      *counter,                     // Counter used for this example
      preserveOutputState,          // preserveOutputState
      status);

   //
   // Validate the Feature Parameters
   //

   // Ensure finite pulse train doesn't overflow counters
   if (numberOfPulsesForFinite > 0xFFFFFFFF/2)
   {
      printf("Error: Requested number of pulses (%u) must be %u or lower.\n", numberOfPulsesForFinite, 0xFFFFFFFF/2);
      status.setCode(kStatusBadParameter);
      return;
   }

   /*********************************************************************\
   |
   |   Program peripheral subsystems
   |
   \*********************************************************************/

   //
   // Configure and route PFI lines
   //

   switch (counterNumber)
   {
   case 0: device.Triggers.PFI_OutputSelectRegister_i[1].writePFI_i_Output_Select(nTriggers::kPFI_G0_Out, &status); break;
   case 1: device.Triggers.PFI_OutputSelectRegister_i[1].writePFI_i_Output_Select(nTriggers::kPFI_G1_Out, &status); break;
   case 2: device.Triggers.PFI_OutputSelectRegister_i[1].writePFI_i_Output_Select(nTriggers::kPFI_G2_Out, &status); break;
   case 3: device.Triggers.PFI_OutputSelectRegister_i[1].writePFI_i_Output_Select(nTriggers::kPFI_G3_Out, &status); break;
   }
   device.Triggers.PFI_Direction_Register.writePFI1_Pin_Dir(nTriggers::kPFI_Output, &status); // Pulse train output terminal

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

   counter->Gi_Mode_Register.setGi_Reload_Source_Switching(nCounter::kUseAlternatingLoadRegisters, &status);
   counter->Gi_Mode_Register.setGi_Loading_On_Gate(nCounter::kNoCounterReloadOnGate, &status);
   counter->Gi_Mode_Register.setGi_ForceSourceEqualToTimebase(kFalse, &status);
   counter->Gi_Mode_Register.setGi_Loading_On_TC(nCounter::kReloadOnTC, &status);
   counter->Gi_Mode_Register.setGi_Output_Mode(nCounter::kToggle_Output_On_TC, &status);
   counter->Gi_Mode_Register.setGi_Load_Source_Select(nCounter::kLoad_From_Register_A, &status);
   switch (mode)
   {
   case kSinglePulse:
   case kFinitePulseTrain:
      counter->Gi_Mode_Register.setGi_Counting_Once(nCounter::kDisarmAtTcThatStops, &status);
      counter->Gi_Mode_Register.setGi_Stop_Mode(nCounter::kStopAtGateOrSecondTC, &status);
      break;
   case kContinuousPulseTrain:
      counter->Gi_Mode_Register.setGi_Counting_Once(nCounter::kNoHardwareDisarm, &status);
      counter->Gi_Mode_Register.setGi_Stop_Mode(nCounter::kStopOnGateCondition, &status);
      break;
   }
   // Gi_Trigger_Mode_For_Edge_Gate doesn't matter
   // Gi_Gate_On_Both_Edges doesn't matter
   counter->Gi_Mode_Register.setGi_Gating_Mode(nCounter::kGateDisabled, &status);
   counter->Gi_Mode_Register.flush(&status);

   counter->Gi_Mode2_Register.setGi_Up_Down(nCounter::kCountDown, &status);
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
   if (mode == kFinitePulseTrain)
   {
      counter->Gi_Counting_Mode_Register.setGi_Counting_Mode(nCounter::kFinitePulseTrain, &status);
   }
   else
   {
      counter->Gi_Counting_Mode_Register.setGi_Counting_Mode(nCounter::kNormalCounting, &status);
   }
   counter->Gi_Counting_Mode_Register.flush(&status);

   // Gi_SampleClockGateIndependent doesn't matter
   // Gi_SampleClockSampleMode doesn't matter
   // Gi_SampleClockPulse doesn't matter
   counter->Gi_SampleClockRegister.setGi_SampleClockMode(nCounter::kSC_Disabled, &status);
   // Gi_SampleClockLevelMode doesn't matter
   // Gi_SampleClockPolarity doesn't matter
   // Gi_SampleClockSelect doesn't matter
   counter->Gi_SampleClockRegister.flush(&status);

   if (mode == kFinitePulseTrain)
   {
      counter->Gi_AuxCtrRegister.writeGi_AuxCtrMode(nCounter::kAux_FinitePulseTrain, &status);
   }
   else
   {
      counter->Gi_AuxCtrRegister.writeGi_AuxCtrMode(nCounter::kAux_Disabled, &status);
   }

   // Gi_Autoincrement_Register doesn't matter

   // Gi_Second_Gate_Polarity doesn't matter
   // Gi_Second_Gate_Select doesn't matter
   counter->Gi_Second_Gate_Register.setGi_Second_Gate_Mode(nCounter::kDisabledSecondGate, &status);
   counter->Gi_Second_Gate_Register.flush(&status);

   counter->Gi_Input_Select_Register.setGi_Source_Polarity(timebasePolarity, &status);
   counter->Gi_Input_Select_Register.setGi_Output_Polarity(outputPolarity, &status);
   // Gi_Gate_Select_Load_Source doesn't matter
   // Gi_Gate_Select doesn't matter
   counter->Gi_Input_Select_Register.setGi_Source_Select(timebaseSource, &status);
   // Gi_Gate_Polarity doesn't matter
   counter->Gi_Input_Select_Register.flush(&status);

   // Gi_ABZ_Select_Register doesn't matter

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
   |   Program output data
   |
   \*********************************************************************/

   //
   // Create, scale, and print output pulse specifications
   //

   // timeSpecs[0] doesn't matter since the counter will use activeTime after the delay
   timeSpecs[1] = delayTime;
   timeSpecs[2] = activeTime;
   timeSpecs[3] = idleTime;

   nNISTC3::nGPCTDataHelper::scaleData(timeSpecs, totalNumberOfSamples,
                                       pulseSpecs, totalNumberOfSamples,
                                       scaler);

   nNISTC3::nGPCTDataHelper::printHeader(generationLabel, "   Active        Idle");
   if (printTime)
   {
      nNISTC3::nGPCTDataHelper::printData(timeSpecs, totalNumberOfSamples, 2);
   }
   else
   {
      nNISTC3::nGPCTDataHelper::printData(pulseSpecs, totalNumberOfSamples, 2);
   }
   printf("\n");

   /*********************************************************************\
   |
   |   Start the pulse train generation
   |
   \*********************************************************************/

   //
   // Prime the counter
   //

   // Preload the initial number of ticks into the main counter
   counter->Gi_Load_A_Register.writeRegister(pulseSpecs[1], &status); // delayTime
   counter->Gi_Command_Register.writeGi_Load(kTrue, &status);

   if (mode == kFinitePulseTrain)
   {
      // The Main counter is counting TCs of the Aux
      if ( (counter->Gi_Input_Select_Register.getGi_Source_Select(&status) == nCounter::kSrc_TB3 ||
            counter->Gi_Mode_Register.getGi_ForceSourceEqualToTimebase (&status) == 1) &&
            pulseSpecs[2] == 2 )
      {
         // If the active number of ticks is only 2, it is necessary to program one more TC than expected
         counter->Gi_Load_A_Register.writeRegister(numberOfPulsesForFinite*2, &status);
      }
      else
      {
         counter->Gi_Load_A_Register.writeRegister(numberOfPulsesForFinite*2-1, &status);
      }
      counter->Gi_Load_B_Register.writeRegister(0, &status);

      // The Aux counter is generating the pulse train
      // Preloading doesn't cause a switch, so the idle number of ticks goes into A since the initial delay
      // is counted in the main counter.
      counter->Gi_AuxCtrLoadA_Register.writeRegister(pulseSpecs[2], &status); // activeTime
      counter->Gi_AuxCtrRegister.writeGi_AuxCtrLoad(kTrue, &status);
      counter->Gi_AuxCtrLoadA_Register.writeRegister(pulseSpecs[3], &status); // idleTime
      counter->Gi_AuxCtrLoadB_Register.writeRegister(pulseSpecs[2], &status); // activeTime
   }
   else
   {
      // Preloading doesn't cause a switch, so the active number of ticks goes into A
      counter->Gi_Load_A_Register.writeRegister(pulseSpecs[2], &status); // activeTime
      counter->Gi_Load_B_Register.writeRegister(pulseSpecs[3], &status); // idleTime
   }

   //
   // Arm and start the counter subsystem
   //

   switch (mode)
   {
   case kSinglePulse:          printf("Generating on-demand pulse.\n", runTime); break;
   case kFinitePulseTrain:     printf("Generating on-demand %u-pulse finite pulse train.\n", numberOfPulsesForFinite); break;
   case kContinuousPulseTrain: printf("Generating on-demand continuous pulse train for %.2f seconds.\n", runTime); break;
   }
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
   |   Wait for the generation to complete
   |
   \*********************************************************************/

   start = clock();
   while (counter->Gi_Status_Register.readGi_Armed_St(&status) == nCounter::kArmed && elapsedTime < runTime)
   {
      // Wait for the run time to expire for 'continuous' pulse train or
      // wait for the pulse to finish generating and disarm the counter.
      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
   }

   /*********************************************************************\
   |
   |   Check for counter subsystem errors
   |
   \*********************************************************************/

   // The counter is not used in a way that generates errors

   /*********************************************************************\
   |
   |   Stop the pulse train generation
   |
   \*********************************************************************/

   //
   // Stop the counter subsystem
   //

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
   |   Finalize the generation
   |
   \*********************************************************************/

   //
   // Print run-time summary
   //

   printf("Finished generating on-demand pulse train.\n", runTime);
   if (mode == kFinitePulseTrain && counter->Gi_Save_Register.readRegister(&status) != 0)
   {
      countsLeft = counter->Gi_Save_Register.getRegister(&status)/2;
      generatedPulses = numberOfPulsesForFinite - countsLeft;
      printf("Generated %u pulses before timeout expired (%u were not generated).\n", generatedPulses, countsLeft);
   }

   //
   // Restore the state of the device
   //

   // Nothing else to do: the helpers' destructors safely unwind device state.
}
