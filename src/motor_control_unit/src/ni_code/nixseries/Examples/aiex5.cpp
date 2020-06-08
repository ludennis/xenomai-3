/*
 * aiex5.cpp
 *   Continuous single-wire hardware-timed analog input with optimized DMA
 *
 * aiex5 reads and scales analog data using hardware timing and transfers data
 * to the host using an optimized DMA buffer. The sample and convert clock
 * share the same external source, which is known as single-wire mode.
 * After configuring the AI subsystem's timing and channel parameters, aiex5
 * configures and starts the DMA channel before sending a software start
 * trigger. Once the measurement starts, every four clock ticks will trigger
 * the sample clock as well as the convert clock for each of the four channels.
 * For 10 seconds, data is read, scaled, and printed in chunks until aiex5
 * stops the timing engine, shuts down DMA, and flushes the buffer. Finally,
 * aiex5 restores the hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : voltage measurement
 * Channel
 *   Channels        : ai0, ai1, ai2, ai3
 *   Scaling         : Volts (*) or raw ADC codes
 *   Terminal config : RSE (*), differential, non-referenced single-ended
 *   Input range     : +/- 10 V (*),  +/- 5 V,     +/- 2 V,    +/- 1 V,
 *                     +/- 500mV,     +/- 200 mV,  +/- 100 mV
 * Timing
 *   Sample mode     : continuous
 * ! Timing mode     : single-wire hardware-timed
 * ! Clock source    : PFI0
 *   Clock polarity  : rising edge (*) or falling edge
 * Trigger
 *   Start trigger   : software
 *   Reference trig  : none
 *   Pause trigger   : none
 * Read Buffer
 *   Data transfer   : optimized scatter-gather DMA ring
 *   DMA transfer    : the buffer is optimized to maximize throughput
 *   DMA buffer      : overwrite or preserve (*) unread samples
 * Behavior
 *   Runtime         : 10 seconds
 *
 * External Connections
 *   ai0:3           : voltages within +/- 10 V (*) or other specified range
 *   PFI0            : TTL sample clock
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
#include "dataHelper.h"
#include "devices.h"
#include "eepromHelper.h"
#include "inTimer/aiHelper.h"
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
   const u32 numberOfChannels = 4;                               // Channels: ai0:3
   tBoolean printVolts = kTrue;
   const nNISTC3::tInputRange range = nNISTC3::kInput_10V;       // Range: +/- 10 Volts
   const nAI::tAI_Config_Channel_Type_t termConfig = nAI::kRSE;  // Term config: Single-ended (aiN vs aiGnd)

   // Timing parameters
   const nAI::tAI_StartConvertSelMux_t sampClkSrc = nAI::kStartCnv_PFI0;          // Source: PFI0
   const nAI::tAI_Polarity_t sampClkPolarity = nAI::kActive_High_Or_Rising_Edge;  // Polarity: Active on rising edge

   // Buffer parameters
   const u32 sampsPerChan = 128;
   const u32 dmaBufferFactor = 16;
   tBoolean allowOverwrite = kFalse;

   // Behavior parameters
   const f64 runTime = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Channel parameters
   u16 gain = 0;
   std::vector<nNISTC3::aiHelper::tChannelConfiguration> chanConfig(numberOfChannels, nNISTC3::aiHelper::tChannelConfiguration());
   u32 maxChannel = numberOfChannels;

   // Timing parameters
   nNISTC3::inTimerParams timingConfig;

   // Buffer parameters
   tBoolean dataOverwritten = kFalse; // Has the DMA channel overwritten any unread data?
   u32 bytesRead = 0;      // Bytes read so far from the DMA buffer
   u32 bytesAvailable = 0; // Bytes in the DMA buffer
   const nNISTC3::tDMAChannelNumber dmaChannel = nNISTC3::kAI_DMAChannel;

   const u32 sampleSizeInBytes = sizeof(i16);
   const u32 totalNumberOfSamples = numberOfChannels * sampsPerChan;
   u32 readSizeInBytes = totalNumberOfSamples * sampleSizeInBytes;
   const u32 dmaSizeInBytes = dmaBufferFactor * readSizeInBytes;

   std::vector<i16> rawData(totalNumberOfSamples, 0);
   std::vector<f32> scaledData(totalNumberOfSamples, 0);

   // Behavior parameters
   f64 elapsedTime = 0; // How long has the measurement been running?
   clock_t start;
   const f64 rlpTimeout = 5; // Wait 5 seconds for a register operation before breaking
   clock_t rlpStart;
   f64 rlpElapsedTime;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tBoolean aiErrored = kFalse; // Did the AI subsystem have an error?
   nInTimer::tInTimer_Error_t scanOverrun = nInTimer::kNO_ERROR;
   nInTimer::tInTimer_Error_t adcOverrun = nInTimer::kNO_ERROR;
   nInTimer::tInTimer_Error_t fifoOverflow = nInTimer::kNO_ERROR;
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

   //
   // Create subsystem helpers
   //

   nNISTC3::aiHelper aiHelper(device, deviceInfo->isSimultaneous, status);
   nNISTC3::eepromHelper eepromHelper(device, deviceInfo->isSimultaneous, deviceInfo->numberOfADCs, deviceInfo->numberOfDACs, status);
   nNISTC3::streamHelper streamHelper(device.AIStreamCircuit, device.CHInCh, status);

   //
   // Validate the Feature Parameters
   //

   // Ensure the device is multiplexed
   if (deviceInfo->isSimultaneous)
   {
      printf("Error: Single-wire mode is not a feature of SMIO devices.\n");
      status.setCode(kStatusBadParameter);
      return;
   }

   // Ensure that the number of channels match the number requested
   if (numberOfChannels > deviceInfo->numberOfAIChannels)
   {
      if (deviceInfo->isSimultaneous)
      {
         printf("Error: Number of channels requested (%u) is greater than number on the device (%u).\n",
            numberOfChannels, deviceInfo->numberOfAIChannels);
         status.setCode(kStatusBadParameter);
         return;
      }
      else
      {
         maxChannel = deviceInfo->numberOfAIChannels;
      }
   }

   // Ensure that the range is supported by this board
   gain = deviceInfo->getAI_Gain(range, status);
   if (status.isFatal())
   {
      printf("Error: Unsupported range.\n");
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

   // Set all PFI lines to be input
   device.Triggers.PFI_Direction_Register.writeRegister(0x0, &status);

   /*********************************************************************\
   |
   |   Program the AI subsystem
   |
   \*********************************************************************/

   //
   // Reset the AI subsystem
   //

   aiHelper.reset(status);

   //
   // Program AI timing
   //

   // Enter timing configuration mode
   device.AI.AI_Timer.Reset_Register.writeConfiguration_Start(kTrue, &status);

   // Disable external gating of the sample clock
   aiHelper.programExternalGate(
      nAI::kGate_Disabled,                  // No external pause signal
      nAI::kActive_High_Or_Rising_Edge,     // Don't care
      status);

   // Program the START1 signal (start trigger) to assert from a software rising edge
   aiHelper.programStart1(
      nAI::kStart1_SW_Pulse,                // Set the line to software-driven
      nAI::kActive_High_Or_Rising_Edge,     // Make line active on rising...
      kTrue,                                //   ...edge (not high level)
      status);

   // Program the START signal (sample clock)
   aiHelper.programStart(
      sampClkSrc,
      sampClkPolarity,                      // Make the line active on...
      kTrue,                                //   ...edge (not level)
      status);

   // Program the convert to be the same as START
   aiHelper.programConvert(
      sampClkSrc,
      sampClkPolarity,
      status);

   // Program the sample and convert clock timing specifications
   timingConfig.setAcqLevelTimingMode(nNISTC3::kInTimerContinuous, status); // This is a continuous measurement
   timingConfig.setUseSICounter(kFalse, status);
   timingConfig.setNumberOfSamples(0, status);      // Doesn't matter since this is continuous

   // Push the timing specification to the device
   aiHelper.getInTimerHelper(status).programTiming(timingConfig, status);

   // Program the FIFO width, this method also clears the FIFO
   aiHelper.programFIFOWidth(nAI::kTwoByteFifo, status);

   //
   // Program AI channels
   //

   // Clear configuration FIFO
   aiHelper.getInTimerHelper(status).clearConfigurationMemory(status);

   for (u16 i=0; i<numberOfChannels; ++i)
   {
      // Set channel parameters
      chanConfig[i].isLastChannel = (i == numberOfChannels-1) ? kTrue : kFalse;  // Last channel?
      chanConfig[i].enableDither = nAI::kEnabled;   // Dithering helps increase ADC accuracy
      chanConfig[i].gain = gain;                    // Set the gain (used by the hardware)
      chanConfig[i].channelType = termConfig;       // Set the terminal configuration
      chanConfig[i].bank = nAI::kBank0;             // AI channels 0..15 are on bank0
      chanConfig[i].channel = i%maxChannel;         // aiN where N = i%maxChannel
      chanConfig[i].range = range;                  // Set the range (used by the scaler)

      // Push the channel configuration to the device
      aiHelper.programChannel(chanConfig[i], status);
   }

   // Advance the configuration FIFO
   aiHelper.getInTimerHelper(status).primeConfigFifo(status);

   // Leave timing configuration mode
   device.AI.AI_Timer.Reset_Register.writeConfiguration_End(kTrue, &status);

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
   |   Start the voltage measurement
   |
   \*********************************************************************/

   //
   // Arm the AI subsystem
   //

   aiHelper.getInTimerHelper(status).armTiming(timingConfig, status);

   //
   // Start the AI subsystem
   //

   printf("Starting continuous %.2f-second single-wire hardware-timed analog measurement.\n", runTime);
   printf("Reading %u-sample chunks from the %u-sample DMA buffer.\n", totalNumberOfSamples, dmaSizeInBytes/sampleSizeInBytes);
   aiHelper.getInTimerHelper(status).strobeStart1(status);

   /*********************************************************************\
   |
   |   Read and print data
   |
   \*********************************************************************/

   nNISTC3::nAIDataHelper::printHeader(1, numberOfChannels, printVolts ? "Voltage" : "ADC code");

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

      // 2. If there is enough data in the buffer, read, scale, and print it
      else if (bytesAvailable >= readSizeInBytes)
      {
         dma->read(readSizeInBytes, reinterpret_cast<u8 *>(&rawData[0]), &bytesAvailable, allowOverwrite, &dataOverwritten, status);
         if (status.isNotFatal())
         {
            if (printVolts)
            {
               nNISTC3::nAIDataHelper::scaleData(rawData, totalNumberOfSamples,
                                                 scaledData, totalNumberOfSamples,
                                                 chanConfig, eepromHelper, *deviceInfo);
               nNISTC3::nAIDataHelper::printData(scaledData, totalNumberOfSamples, numberOfChannels);
            }
            else
            {
               nNISTC3::nAIDataHelper::printData(rawData, totalNumberOfSamples, numberOfChannels);
            }
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

      // 3. Check for AI subsystem errors
      device.AI.AI_Timer.Status_1_Register.refresh(&status);
      scanOverrun = device.AI.AI_Timer.Status_1_Register.getScanOverrun_St(&status);
      adcOverrun = device.AI.AI_Timer.Status_1_Register.getOverrun_St(&status);
      if (deviceInfo->isSimultaneous)
      {
         fifoOverflow = static_cast<nInTimer::tInTimer_Error_t>(device.SimultaneousControl.InterruptStatus.readAiFifoOverflowInterruptCondition(&status));
      }
      else
      {
         fifoOverflow = device.AI.AI_Timer.Status_1_Register.getOverflow_St(&status);
      }

      if (scanOverrun || adcOverrun || fifoOverflow)
      {
         aiErrored = kTrue;
      }

      if (aiErrored || dmaErrored)
      {
         break;
      }

      // 4. Update the run time for this measurement
      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
   }

   /*********************************************************************\
   |
   |   Stop the voltage measurement
   |
   \*********************************************************************/

   if (!aiErrored)
   {
      // Stop and disarm the AI timing engine
      device.AI.AI_Timer.Command_Register.writeEnd_On_End_Of_Scan(kTrue, &status);

      // Wait for the last scan to complete
      rlpElapsedTime = 0;
      rlpStart = clock();
      while (device.AI.AI_Timer.Status_1_Register.readSC_Armed_St(&status))
      {
         // Spin on the SC Armed bit
         if (rlpElapsedTime > rlpTimeout)
         {
            printf("\n");
            printf("Error: AI timing engine did not stop within timeout.\n");
            status.setCode(kStatusRLPTimeout);
            return;
         }
         rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
      }

      // Wait for the Stream Circuit to empty its FIFO
      rlpElapsedTime = 0;
      rlpStart = clock();
      while (!streamHelper.fifoIsEmpty(status))
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
         if (printVolts)
         {
            nNISTC3::nAIDataHelper::scaleData(rawData, readSizeInBytes/sampleSizeInBytes,
                                              scaledData, readSizeInBytes/sampleSizeInBytes,
                                              chanConfig, eepromHelper, *deviceInfo);
            nNISTC3::nAIDataHelper::printData(scaledData, readSizeInBytes/sampleSizeInBytes, numberOfChannels);
         }
         else
         {
            nNISTC3::nAIDataHelper::printData(rawData, readSizeInBytes/sampleSizeInBytes, numberOfChannels);
         }
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
   if (fifoOverflow)
   {
      printf("Error: AI FIFO overflow.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (scanOverrun)
   {
      printf("Error: AI sample clock overrun.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (adcOverrun)
   {
      printf("Error: AI ADC overrun.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (! (aiErrored || dmaErrored))
   {
      printf("Finished continuous %.2f-second single-wire hardware-timed analog measurement.\n", runTime);
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
