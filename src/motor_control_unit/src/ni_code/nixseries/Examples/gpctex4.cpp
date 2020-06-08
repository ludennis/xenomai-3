/*
 * gpctex4.cpp
 *   Finite hardware-timed position measurement with digital filtering
 *
 * gpctex4 measures an encoder using hardware timing and transfers data
 * to the host using DMA. Before configuring the counter, gpctex4 programs the
 * digital filter on each input terminal. After configuring the counter's
 * timing and gating parameters, gpctex4 configures and starts the DMA channel
 * before arming the counter via software. Once the measurement is complete,
 * gpctex4 disarms the counter, shuts down DMA, and reads, scales, and prints
 * the data. Finally, gpctex4 restores the hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : position measurement
 * ! Digital filter  : enabled (*) or disabled
 * ! Filter timebase : on-board oscillator (*) or external PFI line
 * ! Filter width    : none, sync to 100 MHz timebase, small (>100 ns),
 *                     medium (>6.425 us) (*), large (>2.56 ms), or custom count
 * ! Filter clk src  : PFI3 (for custom count)
 * Channel
 *   Channels        : ctr0 (*), ctr1, ctr2 or ctr3
 * ! Scaling         : degrees (*), millimeters,  or raw counts
 * ! Encoder class   : angular (*) or linear
 * ! Encoder type    : X1 (*), X2, X4, or two-pulse
 * ! Z Index         : enabled or disabled (*)
 * ! Z Index mode    : A low,  B low (*); A low,  B high;
 *                     A high, B low;     A high, B high
 * ! Input terminals : PFI0 (A), PFI1 (B), PFI2 (Z)
 * Timing
 * ! Sample mode     : finite
 *   Timing mode     : hardware-timed
 *   Clock source    : PFI4
 *   Clock polarity  : rising edge (*) or falling edge
 * Trigger
 *   Start trigger   : software
 *   Reference trig  : not supported
 *   Pause trigger   : none
 * Read Buffer
 *   Data transfer   : scatter-gather DMA
 *   DMA buffer      : preserve unread samples
 * Behavior
 *   Timeout         : 10 seconds
 *
 * External Connections
 *   PFI0            : 'A' signal from encoder
 *   PFI1            : 'B' signal from encoder
 *   PFI2            : 'Z' signal from encoder
 *   PFI3            : TTL filter clock (if using custom filter timebase)
 *   PFI4            : TTL sample clock
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

   enum tEncoderType
   {
      kX1 = 0,
      kX2,
      kX4,
      kTwoPulse
   };

   //
   // Feature Parameters (modify these to exercise the example)
   //

   // Device parameters
   tBoolean enableDigitalFilters = kTrue;
   nTriggers::tTrig_Filter_Custom_Timebase_t filterTimebase = nTriggers::kFilter_Timebase_TB3;
   const nTriggers::tTrig_Filter_Select_t filterType = nTriggers::kMedium_Filter;
   u16 filterPeriodCount = 500; // Used with a custom filter

   // Channel parameters
   const u32 counterNumber = 0;
   tBoolean printPosition = kTrue;
   const tBoolean encoderIsAngular = kTrue;
   const u32 pulsesPerRevolution = 24;
   const u32 distancePerPulse = 1; // In millimeters
   const tEncoderType encoderType = kX1;
   tBoolean encoderHasZIndex = kFalse;
   const nCounter::tGi_Index_Phase_t zIndexPhase = nCounter::kA_low_B_low;

   // Timing parameters
   const nCounter::tGi_Polarity_t sampClkPolarity = nCounter::kActiveHigh;

   // Buffer parameters
   u32 sampsPerChan = 512;

   // Behavior parameters
   const f64 timeout = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Device parameters
   tCounter* counter = NULL;
   tStreamCircuitRegMap* streamCircuit = NULL;
   const u16 maxPeriodCount = (nTriggers::nTrig_Filter_Settings1_Register::nTrig_Filter_Custom_Period_1::kMask) >>
                               nTriggers::nTrig_Filter_Settings1_Register::nTrig_Filter_Custom_Period_1::kOffset;

   // Channel parameters
   nNISTC3::nGPCTDataHelper::tEncoderScaler_t scaler;
   scaler.isAngular = encoderIsAngular;
   scaler.pulsesPerRevolution = pulsesPerRevolution;
   scaler.distancePerPulse = distancePerPulse;
   nCounter::tGi_CountingMode_t countingMode;
   nCounter::tGi_Index_Mode_t indexMode;

   switch (encoderType)
   {
   case kX1: countingMode = nCounter::kQuadEncoderX1; break;
   case kX2: countingMode = nCounter::kQuadEncoderX2; break;
   case kX4: countingMode = nCounter::kQuadEncoderX4; break;
   case kTwoPulse: countingMode = nCounter::kTwoPulseEncoder; break;
   default:
      printf("Error: Invalid encoder type (%u).\n", encoderType);
      return;
   }

   if (encoderHasZIndex)
   {
      indexMode = nCounter::kIndexModeSet;
   }
   else
   {
      indexMode = nCounter::kIndexModeCleared;
   }

   // Buffer parameters
   tBoolean allowOverwrite = kFalse;
   tBoolean dataOverwritten = kFalse; // Has the DMA channel overwritten any unread data?
   u32 bytesRead = 0;      // Bytes read so far from the DMA buffer
   u32 bytesAvailable = 0; // Bytes in the DMA buffer
   nNISTC3::tDMAChannelNumber dmaChannel;

   const u32 sampleSizeInBytes = sizeof(u32);
   const u32 dmaSizeInBytes = sampsPerChan * sampleSizeInBytes;

   std::vector<u32> rawData(sampsPerChan, 0);
   std::vector<f32> scaledData(sampsPerChan, 0);

   // Behavior parameters
   const tBoolean holdPfiRtsiState = kFalse;
   f64 elapsedTime = 0; // How long has the measurement been running?
   clock_t start;
   const f64 rlpTimeout = 5; // Wait 5 seconds for a register operation before breaking
   clock_t rlpStart;
   f64 rlpElapsedTime;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tBoolean gpctErrored = kFalse; // Did the counter subsystem have an error?
   u32 sampleClockOverrun = 0;
   u32 DRQ_Error = 0;
   tBoolean dmaErrored = kFalse; // Did the DMA channel have an error?
   nMDBG::tStatus2 dmaErr;
   u32 dmaControllerStatus = 0;
   u32 dmaDone = 0; // Has the DMA channel processed the last DMA buffer link?
   tAddressSpace bar0;
   bar0 = bus->createAddressSpace(kPCI_BAR0);

   char* measurementLabel = NULL;
   char* angularLabel = "Angular pos";
   char* linearLabel = "Linear pos";
   char* scaledLabel = NULL;
   char* unitsLabel = NULL;
   char* ticksLabel = "   Ticks";
   char* degreesLabel = "    deg";
   char* displacementLabel = "     mm";
   measurementLabel = scaler.isAngular ? angularLabel : linearLabel;
   scaledLabel = scaler.isAngular ? degreesLabel : displacementLabel;
   unitsLabel = printPosition ? scaledLabel : ticksLabel;

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

   nNISTC3::pfiRtsiResetHelper pfiRtsiResetHelper(device.Triggers, holdPfiRtsiState, status);
   nNISTC3::streamHelper streamHelper(*streamCircuit, device.CHInCh, status);

   nNISTC3::counterResetHelper counterResetHelper(
      *counter,                     // Counter used for this example
      kFalse,                       // preserveOutputState
      status);

   //
   // Validate the Feature Parameters
   //

   // Ensure at least two samples
   if (sampsPerChan < 2)
   {
      printf("Error: Samples per channel (%u) must be 2 or greater.\n", sampsPerChan);
      status.setCode(kStatusBadParameter);
      return;
   }

   // Coerce the filter count if it's too large
   if (filterPeriodCount > maxPeriodCount)
   {
      printf("Warning: filterPeriodCount too large (%u), coercing to %u.\n", filterPeriodCount, maxPeriodCount);
      filterPeriodCount = maxPeriodCount;
   }

   /*********************************************************************\
   |
   |   Program peripheral subsystems
   |
   \*********************************************************************/

   //
   // Configure and route PFI lines
   //

   device.Triggers.PFI_Direction_Register.writePFI0_Pin_Dir(nTriggers::kPFI_Input, &status); // A terminal
   device.Triggers.PFI_Direction_Register.writePFI1_Pin_Dir(nTriggers::kPFI_Input, &status); // B terminal
   device.Triggers.PFI_Direction_Register.writePFI4_Pin_Dir(nTriggers::kPFI_Input, &status); // Sample clock terminal

   if (encoderHasZIndex)
   {
      device.Triggers.PFI_Direction_Register.writePFI2_Pin_Dir(nTriggers::kPFI_Input, &status); // Z terminal
   }

   //
   // Program digital filters
   //

   if (enableDigitalFilters)
   {
      // Select the filter type (note that each PFI line can have its own filter)
      device.Triggers.PFI_Filter_Register_0.setPFI0_Filter_Select(filterType, &status);
      device.Triggers.PFI_Filter_Register_0.setPFI1_Filter_Select(filterType, &status);
      device.Triggers.PFI_Filter_Register_0.setPFI2_Filter_Select(filterType, &status);
      device.Triggers.PFI_Filter_Register_0.flush(&status);

      // Set the custom filter (note that two custom filters may be used at the same time)
      switch (filterType)
      {
      case nTriggers::kCustom_Filter_1:
         device.Triggers.Trig_Filter_Settings1_Register.setTrig_Filter_Custom_Period_1(filterPeriodCount, &status);
         device.Triggers.Trig_Filter_Settings1_Register.setTrig_Filter_Custom_Timebase_1(filterTimebase, &status);
         device.Triggers.Trig_Filter_Settings1_Register.flush(&status);
         break;

      case nTriggers::kCustom_Filter_2:
         device.Triggers.Trig_Filter_Settings2_Register.setTrig_Filter_Custom_Period_2(filterPeriodCount, &status);
         device.Triggers.Trig_Filter_Settings2_Register.setTrig_Filter_Custom_Timebase_2(filterTimebase, &status);
         device.Triggers.Trig_Filter_Settings2_Register.flush(&status);
         break;

      default:
         break;
      }

      // Use Internal Trigger A0 for an external filter timebase, and import it from PFI3
      if (filterTimebase == nTriggers::kExternal_Signal)
      {
         device.Triggers.PFI_Direction_Register.writePFI3_Pin_Dir(nTriggers::kPFI_Input, &status); // Trigger clock terminal
         device.Triggers.Trig_Filter_Settings1_Register.writeTrig_Filter_Ext_Signal_Select(nTriggers::kFilter_Ext_Signal_IntTriggerA0, &status);
         device.Triggers.IntTriggerA_OutputSelectRegister_i[0].writeIntTriggerA_i_Output_Select(nTriggers::kIntTriggerA_PFI3, &status);
      }
   }
   else
   {
      device.Triggers.PFI_Filter_Register_0.setPFI0_Filter_Select(nTriggers::kNo_Filter, &status);
      device.Triggers.PFI_Filter_Register_0.setPFI1_Filter_Select(nTriggers::kNo_Filter, &status);
      device.Triggers.PFI_Filter_Register_0.setPFI2_Filter_Select(nTriggers::kNo_Filter, &status);
      device.Triggers.PFI_Filter_Register_0.flush(&status);
   }

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
   counter->Gi_Mode_Register.setGi_Output_Mode(nCounter::kToggle_Output_On_TC, &status);
   counter->Gi_Mode_Register.setGi_Load_Source_Select(nCounter::kLoad_From_Register_A, &status);
   // Gi_Stop_Mode doesn't matter
   counter->Gi_Mode_Register.setGi_Trigger_Mode_For_Edge_Gate(nCounter::kGateLoads, &status);
   // Gi_Gate_On_Both_Edges doesn't matter
   counter->Gi_Mode_Register.setGi_Gating_Mode(nCounter::kGateDisabled, &status);
   counter->Gi_Mode_Register.flush(&status);

   counter->Gi_Mode2_Register.setGi_Up_Down(nCounter::kGi_B_High_Up, &status);
   counter->Gi_Mode2_Register.setGi_Bank_Switch_Enable(nCounter::kDisabled_If_Armed_Else_Write_To_X, &status);
   counter->Gi_Mode2_Register.setGi_Bank_Switch_Mode(nCounter::kGate, &status);
   // Gi_WriteOnSwitchRequest doesn't matter
   counter->Gi_Mode2_Register.setGi_StopOnError(kTrue, &status);
   // Gi_CtrOutFifoRegenerationEn doesn't matter
   // Gi_HwArmSyncMode doesn't matter
   counter->Gi_Mode2_Register.flush(&status);

   // Gi_Prescale_Div_2 doesn't matter
   counter->Gi_Counting_Mode_Register.setGi_Prescale(kFalse, &status);
   // Gi_HW_Arm_Select doesn't matter
   counter->Gi_Counting_Mode_Register.setGi_HW_Arm_Enable(kFalse, &status);
   if (encoderHasZIndex)
   {
      counter->Gi_Counting_Mode_Register.setGi_Index_Phase(zIndexPhase, &status);
      counter->Gi_Counting_Mode_Register.setGi_Index_Mode(indexMode, &status);
   }
   // Gi_HW_Arm_Polarity doesn't matter
   counter->Gi_Counting_Mode_Register.setGi_Counting_Mode(countingMode, &status);
   counter->Gi_Counting_Mode_Register.flush(&status);

   counter->Gi_SampleClockRegister.setGi_SampleClockGateIndependent(kTrue, &status);
   counter->Gi_SampleClockRegister.setGi_SampleClockSampleMode(nCounter::kSC_LastSaved, &status);
   // Gi_SampleClockPulse doesn't matter
   counter->Gi_SampleClockRegister.setGi_SampleClockMode(nCounter::kSC_SingleSample, &status);
   // Gi_SampleClockLevelMode doesn't matter
   counter->Gi_SampleClockRegister.setGi_SampleClockPolarity(sampClkPolarity, &status);
   counter->Gi_SampleClockRegister.setGi_SampleClockSelect(nCounter::kSampleClk_PFI4, &status);
   counter->Gi_SampleClockRegister.flush(&status);

   counter->Gi_AuxCtrRegister.writeGi_AuxCtrMode(nCounter::kAux_Disabled, &status);

   // Gi_Autoincrement_Register doesn't matter

   // Gi_Second_Gate_Polarity doesn't matter
   // Gi_Second_Gate_Select doesn't matter
   counter->Gi_Second_Gate_Register.setGi_Second_Gate_Mode(nCounter::kDisabledSecondGate, &status);
   counter->Gi_Second_Gate_Register.flush(&status);

   // Gi_Input_Select_Register doesn't matter

   if (encoderHasZIndex)
   {
      counter->Gi_ABZ_Select_Register.setGi_Z_Select(nCounter::kZ_PFI2, &status);
   }
   counter->Gi_ABZ_Select_Register.setGi_B_Select(nCounter::kB_PFI1, &status);
   counter->Gi_ABZ_Select_Register.setGi_A_Select(nCounter::kA_PFI0, &status);
   counter->Gi_ABZ_Select_Register.flush(&status);

   //
   // Program the FIFO
   //

   // Gi_DoneNotificationEnable doesn't matter
   counter->Gi_DMA_Config_Register.setGi_WrFifoEnable(kFalse, &status);
   // Gi_WaitForFirstEventOnGate doesn't matter
   // Gi_DMA_Reset doesn't matter
   counter->Gi_DMA_Config_Register.setGi_DMA_Write(kFalse, &status);
   counter->Gi_DMA_Config_Register.setGi_DMA_Enable(kTrue, &status);
   counter->Gi_DMA_Config_Register.flush(&status);

   /*********************************************************************\
   |
   |   Program DMA
   |
   \*********************************************************************/

   std::auto_ptr<nNISTC3::tCHInChDMAChannel> dma(new nNISTC3::tCHInChDMAChannel(device, dmaChannel, status));
   if (status.isFatal())
   {
      printf("Error: DMA channel initialization (%d).\n", status);
      return;
   }
   dma->reset(status);

   dma->configure(bus, nNISTC3::kLinkChain, nNISTC3::kIn, dmaSizeInBytes, status);
   if (status.isFatal())
   {
      printf("Error: DMA channel configuration (%d).\n", status);
      return;
   }
   dma->start(status);
   if (status.isFatal())
   {
      printf("Error: DMA channel start (%d).\n", status);
      return;
   }

   streamHelper.configureForInput(kTrue, dmaChannel, status);
   streamHelper.modifyTransferSize(dmaSizeInBytes, status);
   streamHelper.enable(status);

   /*********************************************************************\
   |
   |   Start the position measurement
   |
   \*********************************************************************/

   //
   // Prime the counter
   //

   counter->Gi_Load_A_Register.writeRegister(0, &status);
   counter->Gi_Command_Register.writeGi_Load(kTrue, &status);

   //
   // Arm and start the counter subsystem
   //

   printf("Starting finite hardware-timed position measurement.\n");
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
   |   Wait for the measurement to complete
   |
   \*********************************************************************/

   // Wait for the DMA channel to finish processing the last DMA buffer link
   start = clock();
   while (!dmaDone)
   {
      // Check for counter subsystem errors
      counter->Gi_Status_Register.refresh(&status);
      sampleClockOverrun = counter->Gi_Status_Register.getGi_SampleClockOverrun_St(&status);
      DRQ_Error = counter->Gi_Status_Register.getGi_DRQ_Error(&status);

      if (sampleClockOverrun || DRQ_Error)
      {
         gpctErrored = kTrue;
         break;
      }

      dmaControllerStatus = dma->getControllerStatus(status);
      dmaDone = (dmaControllerStatus & nDMAController::nChannel_Status_Register::nLast_Link::kMask) >> nDMAController::nChannel_Status_Register::nLast_Link::kOffset;

      if (elapsedTime > timeout)
      {
         printf("Error: Measurement did not complete within %.2f second timeout.\n", timeout);
         status.setCode(kStatusRLPTimeout);
         return;
      }
      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
   }

   /*********************************************************************\
   |
   |   Stop the position measurement
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
   |   Read and print the data
   |
   \*********************************************************************/

   //
   // Disable DMA on the device and stop the CHInCh DMA channel
   //

   streamHelper.disable(status);
   dma->stop(status);

   //
   // Read and print the data from the DMA buffer
   //

   nNISTC3::nGPCTDataHelper::printHeader(measurementLabel, unitsLabel);

   // 1. Use the read() method to query the number of bytes in the DMA buffer
   dma->read(0, NULL, &bytesAvailable, allowOverwrite, &dataOverwritten, status);
   if (status.isFatal())
   {
      dmaErr = status;
      dmaErrored = kTrue;
   }

   // 2. If there is enough data in the buffer, read, scale, and print it
   else if (bytesAvailable >= dmaSizeInBytes)
   {
      dma->read(dmaSizeInBytes, reinterpret_cast<u8 *>(&rawData[0]), &bytesAvailable, allowOverwrite, &dataOverwritten, status);
      if (status.isNotFatal())
      {
         if (printPosition)
         {
            nNISTC3::nGPCTDataHelper::scaleData(rawData, sampsPerChan, scaledData, sampsPerChan, scaler);
            nNISTC3::nGPCTDataHelper::printData(scaledData, sampsPerChan, 1);
         }
         else
         {
            nNISTC3::nGPCTDataHelper::printData(rawData, sampsPerChan, 1);
         }
         bytesRead += dmaSizeInBytes;
      }
      else
      {
         dmaErr = status;
         dmaErrored = kTrue;
      }
   }
   printf("\n");

   /*********************************************************************\
   |
   |   Finalize the measurement
   |
   \*********************************************************************/

   //
   // Print run-time summary
   //

   if (dmaErrored)
   {
      printf("Error: DMA read (%d).\n", dmaErr);
      status.setCode(kStatusRuntimeError);
   }
   if (sampleClockOverrun)
   {
      printf("Error: Sample clock overrun.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (DRQ_Error)
   {
      printf("Error: DMA request error, most likely a FIFO overflow.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (! (gpctErrored || dmaErrored))
   {
      printf("Finished finite hardware-timed position measurement.\n");
      printf("Read %u samples (%s) using a %u-sample DMA buffer.\n",
             bytesRead/sampleSizeInBytes,
             dataOverwritten ? "by overwriting data" : "without overwriting data",
             dmaSizeInBytes/sampleSizeInBytes);
   }

   //
   // Restore the state of the device
   //

   // Nothing else to do: the helpers' destructors safely unwind device state.
}
