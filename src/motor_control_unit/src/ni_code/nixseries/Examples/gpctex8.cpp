/*
 * gpctex8.cpp
 *   Finite implicitly-timed pulse train output with start trigger
 *
 * gpctex8 scales and generates a finite pulse train using implicit timing
 * and transfers data to the device using DMA. After configuring the counter's
 * timing and gating parameters, gpctex8 creates a simple 'chirp' pulse train
 * to generate and configures and primes the DMA channel before waiting 10
 * seconds for an external start trigger. Once triggered, the counter generates
 * the pulses, following the specified idle count with the specified active
 * count. After the generation is complete, gpctex8 disarms the counter and
 * shuts down DMA. Finally, gpctex8 restores the hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : pulse train generation
 * Channel
 *   Channels        : ctr0 (*), ctr1, ctr2 or ctr3
 *   Scaling         : time (*) or raw counts
 *   Output terminal : PFI0
 *   Output polarity : active low (*) or active high
 *   Output state    : preserve on exit (*) or tristate
 * Timing
 * ! Sample mode     : finite
 * ! Timing mode     : implicit
 *   Clock source    : on-board oscillator
 * Trigger
 * ! Start trigger   : PFI1 (digital rising edge)
 *   Reference trig  : not supported
 *   Pause trigger   : none
 * Write Buffer
 * ! Data transfer   : scatter-gather DMA
 *   DMA buffer      : wait for new data
 * Behavior
 *   Trig wait time  : 10 seconds
 *
 * External Connections
 *   PFI0            : observe on an oscilloscope
 *   PFI1            : TTL start trigger
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include <memory> // For auto_ptr<T>
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
#include "streamHelper.h"

// DMA Support
#include "CHInCh/dmaErrors.h"
#include "CHInCh/dmaProperties.h"
#include "CHInCh/tCHInChDMAChannel.h"

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
   tBoolean printTime = kTrue;
   const nCounter::tGi_Polarity_t outputPolarity = nCounter::kActiveLow;
   tBoolean preserveOutputState = kTrue;

   // Trigger parameters
   const nCounter::tGi_Polarity_t startTrigPolarity = nCounter::kActiveHigh;

   // Buffer parameters
   u32 numberOfPulses = 10;

   // Behavior parameters
   const f64 triggerWaitTime = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Device parameters
   tCounter* counter = NULL;
   tStreamCircuitRegMap* streamCircuit = NULL;

   // Channel parameters
   nNISTC3::nGPCTDataHelper::tPulseTrainScaler_t scaler;
   scaler.timebaseRate = 100000000;

   // Buffer parameters
   const tBoolean allowRegeneration = kFalse;
   tBoolean dataRegenerated = kFalse; // Has the DMA channel regenerated old data?
   u32 availableSpace = 0; // Space in the DMA buffer
   nNISTC3::tDMAChannelNumber dmaChannel;

   const u32 sampleSizeInBytes = sizeof(u32);
   const u32 totalNumberOfSamples = 2*numberOfPulses; // Each pulse is a pair of samples: idle tick count and active tick count
   const u32 dmaSizeInBytes = sampleSizeInBytes * totalNumberOfSamples;

   std::vector<f32> timeSpecs(totalNumberOfSamples, 0.1F);
   std::vector<u32> pulseSpecs(totalNumberOfSamples, 1); // The counter must count at least two counts

   // Behavior parameters
   f64 elapsedTime = 0; // How long has the counter been waiting for the start trigger?
   clock_t start;
   const f64 rlpTimeout = 5; // Wait 5 seconds for a register operation before breaking
   clock_t rlpStart;
   f64 rlpElapsedTime;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tBoolean gpctErrored = kFalse; // Did the counter subsystem have an error?
   u32 auxTC_Error = 0;
   u32 writesTooFastError = 0;
   u32 gateSwitchError = 0;
   u32 DRQ_Error = 0;
   u32 done = 0; // Has the counter finished generating the pulse train?
   tAddressSpace bar0;
   bar0 = bus->createAddressSpace(kPCI_BAR0);

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
   case 0:
      dmaChannel = nNISTC3::kCounter0DmaChannel;
      counter = &device.Counter0;
      streamCircuit = &device.Counter0StreamCircuit;
      break;
   case 1:
      dmaChannel = nNISTC3::kCounter1DmaChannel;
      counter = &device.Counter1;
      streamCircuit = &device.Counter1StreamCircuit;
      break;
   case 2:
      dmaChannel = nNISTC3::kCounter2DmaChannel;
      counter = &device.Counter2;
      streamCircuit = &device.Counter2StreamCircuit;
      break;
   case 3:
      dmaChannel = nNISTC3::kCounter3DmaChannel;
      counter = &device.Counter3;
      streamCircuit = &device.Counter3StreamCircuit;
      break;
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

   nNISTC3::streamHelper streamHelper(*streamCircuit, device.CHInCh, status);

   //
   // Validate the Feature Parameters
   //

   // Ensure at least two pulses
   if (numberOfPulses < 2)
   {
      printf("Error: Number of pulses (%) must be 2 or greater.\n", numberOfPulses);
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
   case 0:
      device.Triggers.PFI_OutputSelectRegister_i[0].writePFI_i_Output_Select(nTriggers::kPFI_G0_Out, &status);
      break;
   case 1:
      device.Triggers.PFI_OutputSelectRegister_i[0].writePFI_i_Output_Select(nTriggers::kPFI_G1_Out, &status);
      break;
   case 2:
      device.Triggers.PFI_OutputSelectRegister_i[0].writePFI_i_Output_Select(nTriggers::kPFI_G2_Out, &status);
      break;
   case 3:
      device.Triggers.PFI_OutputSelectRegister_i[0].writePFI_i_Output_Select(nTriggers::kPFI_G3_Out, &status);
      break;
   }
   device.Triggers.PFI_Direction_Register.writePFI0_Pin_Dir(nTriggers::kPFI_Output, &status); // Pulse train output terminal
   device.Triggers.PFI_Direction_Register.writePFI1_Pin_Dir(nTriggers::kPFI_Input, &status);  // Start trigger terminal

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
   counter->Gi_Mode_Register.setGi_Counting_Once(nCounter::kDisarmAtTcThatStops, &status);
   counter->Gi_Mode_Register.setGi_Output_Mode(nCounter::kToggle_Output_On_TC, &status);
   counter->Gi_Mode_Register.setGi_Load_Source_Select(nCounter::kLoad_From_Register_A, &status);
   counter->Gi_Mode_Register.setGi_Stop_Mode(nCounter::kStopAtGateOrFirstTC, &status);
   counter->Gi_Mode_Register.setGi_Trigger_Mode_For_Edge_Gate(nCounter::kGateEdgeStarts, &status);
   // Gi_Gate_On_Both_Edges doesn't matter
   counter->Gi_Mode_Register.setGi_Gating_Mode(nCounter::kAssertingEdgeGating, &status);
   counter->Gi_Mode_Register.flush(&status);

   counter->Gi_Mode2_Register.setGi_Up_Down(nCounter::kCountDown, &status);
   counter->Gi_Mode2_Register.setGi_Bank_Switch_Enable(nCounter::kDisabled_If_Armed_Else_Write_To_X, &status);
   counter->Gi_Mode2_Register.setGi_Bank_Switch_Mode(nCounter::kSoftware, &status);
   // Gi_WriteOnSwitchRequest doesn't matter
   // Gi_StopOnError doesn't matter
   counter->Gi_Mode2_Register.setGi_CtrOutFifoRegenerationEn(kFalse, &status);
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

   counter->Gi_AuxCtrLoadA_Register.writeRegister(numberOfPulses - 1, &status);

   counter->Gi_AuxCtrRegister.setGi_AuxCtrLoad(kTrue, &status);
   counter->Gi_AuxCtrRegister.setGi_AuxCtrMode(nCounter::kAux_FinitePwmGeneration, &status);
   counter->Gi_AuxCtrRegister.flush(&status);

   // Gi_Autoincrement_Register doesn't matter

   // Gi_Second_Gate_Polarity doesn't matter
   // Gi_Second_Gate_Select doesn't matter
   counter->Gi_Second_Gate_Register.setGi_Second_Gate_Mode(nCounter::kDisabledSecondGate, &status);
   counter->Gi_Second_Gate_Register.flush(&status);

   counter->Gi_Input_Select_Register.setGi_Source_Polarity(nCounter::kActiveHigh, &status);
   counter->Gi_Input_Select_Register.setGi_Output_Polarity(outputPolarity, &status);
   counter->Gi_Input_Select_Register.setGi_Gate_Select_Load_Source(nCounter::kDisabled, &status);
   counter->Gi_Input_Select_Register.setGi_Gate_Select(nCounter::kGate_PFI1, &status);
   counter->Gi_Input_Select_Register.setGi_Source_Select(nCounter::kSrc_TB3, &status);
   counter->Gi_Input_Select_Register.setGi_Gate_Polarity(startTrigPolarity, &status);
   counter->Gi_Input_Select_Register.flush(&status);

   // Gi_ABZ_Select_Register doesn't matter

   //
   // Program the FIFO
   //

   // Gi_DoneNotificationEnable doesn't matter
   counter->Gi_DMA_Config_Register.setGi_WrFifoEnable(kTrue, &status);
   // Gi_WaitForFirstEventOnGate doesn't matter
   counter->Gi_DMA_Config_Register.setGi_DMA_Reset(kTrue, &status);
   counter->Gi_DMA_Config_Register.setGi_DMA_Write(kTrue, &status);
   counter->Gi_DMA_Config_Register.setGi_DMA_Enable(kTrue, &status);
   counter->Gi_DMA_Config_Register.flush(&status);

   /*********************************************************************\
   |
   |   Program DMA
   |
   \*********************************************************************/

   //
   // Create, scale, and print output pulse specifications
   //

   // Generate a pulse train 'chirp' with ~%30 duty cycle
   for (u32 i=0; i<timeSpecs.size(); ++i)
   {
      timeSpecs[i] =   static_cast<f32>(0.05 + 0.02*(i%20)); // Active tick time
      timeSpecs[++i] = static_cast<f32>(0.02 + 0.01*(i%20)); // Idle tick time
   }
   nNISTC3::nGPCTDataHelper::scaleData(timeSpecs, totalNumberOfSamples,
                                       pulseSpecs, totalNumberOfSamples,
                                       scaler);

   nNISTC3::nGPCTDataHelper::printHeader("Pulse spec", "   Active        Idle");
   if (printTime)
   {
      nNISTC3::nGPCTDataHelper::printData(timeSpecs, totalNumberOfSamples, 2);
   }
   else
   {
      nNISTC3::nGPCTDataHelper::printData(pulseSpecs, totalNumberOfSamples, 2);
   }
   printf("\n");

   //
   // Configure DMA channel
   //

   std::auto_ptr<nNISTC3::tCHInChDMAChannel> dma(new nNISTC3::tCHInChDMAChannel(device, dmaChannel, status));
   if (status.isFatal())
   {
      printf("Error: DMA channel initialization (%d).\n", status);
      return;
   }
   dma->reset(status);

   dma->configure(bus, nNISTC3::kLinkChain, nNISTC3::kOut, dmaSizeInBytes, status);
   if (status.isFatal())
   {
      printf("Error: DMA channel configuration (%d).\n", status);
      return;
   }

   //
   // Prime DMA buffer
   //

   dma->write(dmaSizeInBytes, reinterpret_cast<u8 *>(&pulseSpecs[0]), &availableSpace, allowRegeneration, &dataRegenerated, status);
   if (status.isFatal())
   {
      printf("Error: DMA write (%d).\n", status);
      return;
   }

   //
   // Start DMA channel
   //

   dma->start(status);
   if (status.isFatal())
   {
      printf("Error: DMA channel start (%d).\n", status);
      return;
   }

   streamHelper.configureForOutput(kTrue, dmaChannel, status);
   streamHelper.modifyTransferSize(dmaSizeInBytes, status);
   streamHelper.enable(status);

   /*********************************************************************\
   |
   |   Start the pulse train generation
   |
   \*********************************************************************/

   //
   // Wait for data to enter the FIFO
   //

   rlpElapsedTime = 0;
   rlpStart = clock();
   while (counter->Gi_FifoStatusRegister.readRegister(&status) < 1)
   {
      // Spin until the counter FIFO is no longer empty
      if (rlpElapsedTime > rlpTimeout)
      {
         printf("Error: Counter FIFO did not receive data within timeout.\n");
         status.setCode(kStatusRLPTimeout);
         return;
      }
      rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
   }

   //
   // Prime the counter
   //

   // Load banks X::A and X::B (the first pulse) from the buffer
   counter->Gi_Command_Register.writeGi_WrLoadRegsFromFifo(kTrue, &status);
   rlpElapsedTime = 0;
   rlpStart = clock();
   while (counter->Gi_Status_Register.readGi_ForcedWrFromFifoInProgSt(&status))
   {
      // Wait for the counter to finish loading from the FIFO
      if (rlpElapsedTime > rlpTimeout)
      {
         printf("Error: Counter did not load data from FIFO to Bank X within timeout.\n");
         status.setCode(kStatusRLPTimeout);
         return;
      }
      rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
   }

   // Switch to register X::B
   counter->Gi_Mode_Register.setGi_Load_Source_Select(nCounter::kLoad_From_Register_B, &status);
   counter->Gi_Mode_Register.flush(&status);

   // Load register X::B into the counter (the first idle count)
   counter->Gi_Command_Register.writeGi_Load(kTrue, &status);

   // Switch to register X::A
   counter->Gi_Mode_Register.setGi_Load_Source_Select(nCounter::kLoad_From_Register_A, &status);
   counter->Gi_Mode_Register.flush(&status);

   // Switch to bank Y
   counter->Gi_Mode2_Register.setGi_Bank_Switch_Enable(nCounter::kEnabled_If_Armed_Else_Write_To_Y, &status);
   counter->Gi_Mode2_Register.flush(&status);

   // Load banks Y::A and Y::B for the next pulse period
   counter->Gi_Command_Register.writeGi_WrLoadRegsFromFifo(kTrue, &status);
   rlpElapsedTime = 0;
   rlpStart = clock();
   while (counter->Gi_Status_Register.readGi_ForcedWrFromFifoInProgSt(&status))
   {
      // Wait for the counter to finish loading from the FIFO
      if (rlpElapsedTime > rlpTimeout)
      {
         printf("Error: Counter did not load data from FIFO to Bank Y within timeout.\n");
         status.setCode(kStatusRLPTimeout);
         return;
      }
      rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
   }

   //
   // Arm the counter subsystem
   //

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

   //
   // Wait for the start trigger
   //

   elapsedTime = 0;
   start = clock();
   while ( !counter->Gi_Status_Register.readGi_Counting_St(&status) &&
           !counter->Gi_Status_Register.getGi_TC_St(&status) )
   {
      printf("Waiting %.2f seconds for start trigger...\r", triggerWaitTime-elapsedTime);
      if (elapsedTime > triggerWaitTime)
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
   |   Wait for the generation to complete
   |
   \*********************************************************************/

   printf("Generating pulse train.\n");
   done = counter->Gi_Status_Register.readGi_AuxTC_EventSt(&status);
   rlpElapsedTime = 0;
   rlpStart = clock();
   while (!done)
   {
      // Check for counter subsystem errors
      counter->Gi_Status_Register.refresh(&status);
      auxTC_Error = counter->Gi_Status_Register.getGi_AuxTC_ErrorEventSt(&status);
      writesTooFastError = counter->Gi_Status_Register.getGi_WritesTooFastErrorSt(&status);
      gateSwitchError = counter->Gi_Status_Register.getGi_GateSwitchError_St (&status);
      DRQ_Error = counter->Gi_Status_Register.getGi_DRQ_Error(&status);

      if (auxTC_Error || writesTooFastError || gateSwitchError || DRQ_Error)
      {
         gpctErrored = kTrue;
         break;
      }

      // Spin on the terminal count of the aux counter
      done = counter->Gi_Status_Register.readGi_AuxTC_EventSt(&status);

      if (rlpElapsedTime > rlpTimeout)
      {
         printf("Warning: Counter did not generate pulse train within %.2f second timeout.\n", rlpTimeout);
         break;
      }
      rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
   }

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

   //
   // Disable DMA on the device and stop the CHInCh DMA channel
   //

   streamHelper.disable(status);
   dma->stop(status);

   /*********************************************************************\
   |
   |   Finalize the generation
   |
   \*********************************************************************/

   //
   // Print run-time summary
   //

   if (auxTC_Error)
   {
      printf("Error: Aux counter reached terminal count twice without acknowledgement.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (writesTooFastError)
   {
      printf("Error: Counter updated before valid gate switch.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (gateSwitchError)
   {
      printf("Error: Sample clock arrived before counter ready to switch.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (DRQ_Error)
   {
      printf("Error: DMA request error.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (!gpctErrored)
   {
      printf("Finished generating finite pulse train.\n");
      printf("Generated %u-pulse pulse train (%u total samples) %s.\n",
         numberOfPulses,
         totalNumberOfSamples,
         dataRegenerated ? "by regenerating old data" : "without regenerating old data");
   }

   //
   // Restore the state of the device
   //

   // Nothing else to do: the helpers' destructors safely unwind device state.
}
