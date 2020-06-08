/*
 * gpctex5.cpp
 *   Continuous hardware-timed edge counting with optimized DMA
 *
 * gpctex5 counts TTL edges using hardware timing and transfers data to
 * the host using an optimized DMA buffer. After configuring the counter's
 * timing and gating parameters, gpctex5 configures and starts the DMA
 * channel before arming the counter via software. For 10 seconds, data
 * is read and printed in chunks until gpctex5 disarms the counter, shuts
 * down DMA, and flushes the buffer. Finally, gpctex5 restores the hardware's
 * previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : edge counting
 * Channel
 *   Channels        : ctr0 (*), ctr1, ctr2 or ctr3
 *   Scaling         : none
 *   Input terminal  : PFI0
 *   Input polarity  : rising edge (*) or falling edge
 * Timing
 *   Sample mode     : continuous
 *   Timing mode     : hardware-timed
 *   Clock source    : PFI1
 *   Clock polarity  : rising edge (*) or falling edge
 * Trigger
 *   Start trigger   : software
 *   Reference trig  : not supported
 *   Pause trigger   : none
 * Read Buffer
 *   Data transfer   : optimized scatter-gather DMA ring
 *   DMA transfer    : the buffer is optimized to maximize throughput
 *   DMA buffer      : overwrite or preserve (*) unread samples
 * Behavior
 *   Runtime         : 10 seconds
 *
 * External Connections
 *   PFI0            : TTL input signal
 *   PFI1            : TTL sample clock
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
   const nCounter::tGi_Source_Select_t source = nCounter::kSrc_PFI0;
   const nCounter::tGi_Polarity_t sourcePolarity = nCounter::kActiveHigh;

   // Timing parameters
   const nCounter::tGi_SampleClockSelect_t sampClkSource = nCounter::kSampleClk_PFI1;
   const nCounter::tGi_Polarity_t sampClkPolarity = nCounter::kActiveHigh;

   // Buffer parameters
   const u32 sampsPerChan = 512;
   const u32 dmaBufferFactor = 16;
   tBoolean allowOverwrite = kFalse;

   // Behavior parameters
   const f64 runTime = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Device parameters
   tCounter* counter = NULL;
   tStreamCircuitRegMap* streamCircuit = NULL;

   // Buffer parameters
   tBoolean dataOverwritten = kFalse; // Has the DMA channel overwritten any unread data?
   u32 bytesRead = 0;      // Bytes read so far from the DMA buffer
   u32 bytesAvailable = 0; // Bytes in the DMA buffer
   nNISTC3::tDMAChannelNumber dmaChannel;

   const u32 sampleSizeInBytes = sizeof(u32);
   u32 readSizeInBytes = sampsPerChan * sampleSizeInBytes;
   const u32 dmaSizeInBytes = dmaBufferFactor * readSizeInBytes;

   std::vector<u32> rawData(sampsPerChan, 0);

   // Behavior parameters
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

   nNISTC3::streamHelper streamHelper(*streamCircuit, device.CHInCh, status);

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
   counter->Gi_Mode_Register.setGi_Trigger_Mode_For_Edge_Gate(nCounter::kGateLoads, &status);
   // Gi_Gate_On_Both_Edges doesn't matter
   counter->Gi_Mode_Register.setGi_Gating_Mode(nCounter::kGateDisabled, &status);
   counter->Gi_Mode_Register.flush(&status);

   counter->Gi_Mode2_Register.setGi_Up_Down(nCounter::kCountUp, &status);
   counter->Gi_Mode2_Register.setGi_Bank_Switch_Enable(nCounter::kDisabled_If_Armed_Else_Write_To_X, &status);
   counter->Gi_Mode2_Register.setGi_Bank_Switch_Mode(nCounter::kGate, &status);
   // Gi_WriteOnSwitchRequest doesn't matter
   counter->Gi_Mode2_Register.setGi_StopOnError(kFalse, &status);
   // Gi_CtrOutFifoRegenerationEn doesn't matter
   // Gi_HwArmSyncMode doesn't matter
   counter->Gi_Mode2_Register.flush(&status);

   // Gi_Prescale_Div_2 doesn't matter
   counter->Gi_Counting_Mode_Register.setGi_Prescale(kFalse, &status);
   // Gi_HW_Arm_Select doesn't matter
   counter->Gi_Counting_Mode_Register.setGi_HW_Arm_Enable(kFalse, &status);
   // Gi_Index_Phase doesn't matter
   // Gi_Index_Mode doesn't matter
   // Gi_HW_Arm_Polarity doesn't matter
   counter->Gi_Counting_Mode_Register.setGi_Counting_Mode(nCounter::kNormalCounting, &status);
   counter->Gi_Counting_Mode_Register.flush(&status);

   counter->Gi_SampleClockRegister.setGi_SampleClockGateIndependent(kTrue, &status);
   counter->Gi_SampleClockRegister.setGi_SampleClockSampleMode(nCounter::kSC_LastSaved, &status);
   // Gi_SampleClockPulse doesn't matter
   counter->Gi_SampleClockRegister.setGi_SampleClockMode(nCounter::kSC_SingleSample, &status);
   // Gi_SampleClockLevelMode doesn't matter
   counter->Gi_SampleClockRegister.setGi_SampleClockPolarity(sampClkPolarity, &status);
   counter->Gi_SampleClockRegister.setGi_SampleClockSelect(sampClkSource, &status);
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

   // Gi_ABZ_Select_Register doesn't matter

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

   // Use the optimized 2-link SGL ring buffer
   dma->configure(bus, nNISTC3::kReuseLinkRing, nNISTC3::kIn, dmaSizeInBytes, status);
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

   streamHelper.configureForInput(!allowOverwrite, dmaChannel, status);
   if (!allowOverwrite)
   {
      streamHelper.modifyTransferSize(dmaSizeInBytes, status);
   }
   streamHelper.enable(status);

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
   // Arm and start the counter subsystem
   //

   printf("Starting continuous %.2f-second hardware-timed edge counting measurement.\n", runTime);
   printf("Reading %u-sample chunks from the %u-sample DMA buffer.\n", sampsPerChan, dmaSizeInBytes/sampleSizeInBytes);
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

   nNISTC3::nGPCTDataHelper::printHeader("Count Edges", "   Ticks");

   start = clock();
   while (elapsedTime < runTime)
   {
      // 1. Use the read() method to query the number of bytes in the DMA buffer
      dma->read(0, NULL, &bytesAvailable, allowOverwrite, &dataOverwritten, status);
      if (status.isFatal())
      {
         dmaErr = status;
         dmaErrored = kTrue;
      }

      // 2. If there is enough data in the buffer, read and print it
      else if (bytesAvailable >= readSizeInBytes)
      {
         dma->read(readSizeInBytes, reinterpret_cast<u8 *>(&rawData[0]), &bytesAvailable, allowOverwrite, &dataOverwritten, status);
         if (status.isNotFatal())
         {
            nNISTC3::nGPCTDataHelper::printData(rawData, sampsPerChan, 1);
            bytesRead += readSizeInBytes;

            // Permit the Stream Circuit to transfer another readSizeInBytes bytes
            if (!allowOverwrite)
            {
               streamHelper.modifyTransferSize(readSizeInBytes, status);
            }
         }
         else
         {
            dmaErr = status;
            dmaErrored = kTrue;
         }
      }

      // 3. Check for counter subsystem errors
      counter->Gi_Status_Register.refresh(&status);
      sampleClockOverrun = counter->Gi_Status_Register.getGi_SampleClockOverrun_St(&status);
      DRQ_Error = counter->Gi_Status_Register.getGi_DRQ_Error(&status);

      if (sampleClockOverrun || DRQ_Error)
      {
         gpctErrored = kTrue;
      }

      if (gpctErrored || dmaErrored)
      {
         break;
      }

      // 4. Update the run time for this measurement
      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
   }

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
         printf("\n");
         printf("Error: Counter did not disarm within timeout.\n");
         status.setCode(kStatusRLPTimeout);
         return;
      }
      rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
   }

   // Wait for the counter to empty its FIFO
   rlpElapsedTime = 0;
   rlpStart = clock();
   while (counter->Gi_FifoStatusRegister.readRegister(&status))
   {
      // Spin on the sample count in the FIFO
      if (rlpElapsedTime > rlpTimeout)
      {
         printf("\n");
         printf("Error: Counter FIFO did not flush within timeout.\n");
         status.setCode(kStatusRLPTimeout);
         return;
      }
      rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
   }

   // Wait for the Stream Circuit to empty its FIFO
   rlpElapsedTime = 0;
   rlpStart = clock();
   while (! streamHelper.fifoIsEmpty(status))
   {
      // Spin on the FifoEmpty bit
      if (rlpElapsedTime > rlpTimeout)
      {
         printf("\n");
         printf("Error: Stream circuit did not flush within timeout.\n");
         status.setCode(kStatusRLPTimeout);
         return;
      }
      rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
   }

   /*********************************************************************\
   |
   |   Flush the DMA buffer
   |
   \*********************************************************************/

   //
   // Disable DMA on the device and stop the CHInCh DMA channel
   //

   streamHelper.disable(status);
   dma->stop(status);

   //
   // Read remaining samples from the DMA buffer
   //

   dma->read(0, NULL, &bytesAvailable, allowOverwrite, &dataOverwritten, status);
   if (status.isFatal())
   {
      dmaErr = status;
      dmaErrored = kTrue;
   }
   while (bytesAvailable)
   {
      if (bytesAvailable < readSizeInBytes)
      {
         readSizeInBytes = bytesAvailable;
      }
      dma->read(readSizeInBytes, reinterpret_cast<u8 *>(&rawData[0]), &bytesAvailable, allowOverwrite, &dataOverwritten, status);
      if (status.isNotFatal())
      {
         nNISTC3::nGPCTDataHelper::printData(rawData, readSizeInBytes/sampleSizeInBytes, 1);
         bytesRead += readSizeInBytes;
      }
      else
      {
         dmaErr = status;
         dmaErrored = kTrue;
         break;
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
      printf("Finished continuous %.2f-second hardware-timed edge counting measurement.\n", runTime);
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
