/*
 * aiex4.cpp
 *   Finite hardware-timed analog input with reference trigger
 *
 * aiex4 reads and scales analog data using hardware timing and transfers data
 * to the host using DMA. After configuring the reference trigger input and AI
 * subsystem's timing and channel parameters, aiex4 configures and starts the
 * DMA channel before sending a software start trigger. For 10 seconds,
 * aiex4 waits for a reference trigger, counting down how many pre- and post-
 * trigger samples remain in the measurement. Once the measurement is complete,
 * aiex4 shuts down DMA and reads, scales, and prints the data. Finally, aiex4
 * restores the hardware's previous state.

 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : voltage measurement
 * Channel
 *   Channels        : ai0
 *   Scaling         : Volts (*) or raw ADC codes
 *   Terminal config : RSE (*), differential, non-referenced single-ended
 *   Input range     : +/- 10 V (*),  +/- 5 V,     +/- 2 V,    +/- 1 V,
 *                     +/- 500mV,     +/- 200 mV,  +/- 100 mV
 * Timing
 *   Sample mode     : finite
 *   Timing mode     : hardware-timed
 *   Clock source    : on-board oscillator
 *   Clock rate      : 500 Hz
 *   Clock polarity  : rising edge (*) or falling edge
 * Trigger
 *   Start trigger   : software
 * ! Reference trig  : PFI0
 *   Pause trigger   : none
 * Read Buffer
 * ! Data transfer   : scatter-gather DMA ring
 * ! DMA buffer      : overwrite unread samples
 * Behavior
 *   Timeout         : 10 seconds
 *
 * External Connections
 *   ai0             : voltage within +/- 10 V (*) or other specified range
 *   PFI0            : TTL reference trigger
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
   const u32 numberOfChannels = 1;                               // Channels: ai0
   tBoolean printVolts = kTrue;
   const nNISTC3::tInputRange range = nNISTC3::kInput_10V;       // Range: +/- 10 Volts
   const nAI::tAI_Config_Channel_Type_t termConfig = nAI::kRSE;  // Term config: Single-ended (aiN vs aiGnd)

   // Timing parameters
   u32 samplePeriod = 200000; // Rate: 100 MHz / 200e3 = 500 Hz

   // Trigger parameters
   const nAI::tAI_START2_Select_t refTrigSrc = nAI::kStart2_PFI0; // Use PFI0 as the reference trigger
   const nAI::tAI_Polarity_t refTrigPolarity = nAI::kActive_High_Or_Rising_Edge; // Use rising edge

   // Buffer parameters
   u32 preTrigSamps = 100;   // Take 100 samples before the trigger
   u32 postTrigSamps = 1000; // Take 1000 samples after the trigger

   // Behavior parameters
   const f64 timeout = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Channel parameters
   u16 gain = 0;
   std::vector<nNISTC3::aiHelper::tChannelConfiguration> chanConfig(numberOfChannels, nNISTC3::aiHelper::tChannelConfiguration());
   u32 maxChannel = numberOfChannels;

   // Timing parameters
   const u32 sampleDelay = 2; // Wait N TB3 ticks after the start trigger before clocking (N must be >= 2)
   const u32 convertPeriod = 400; // 100 MHz / 400 = 250 kHz (refer to device specs for minimum period)
   const u32 convertDelay = 2; // Wait N TB3 ticks after the sample clock before converting (N must be >= 2)
   nNISTC3::inTimerParams timingConfig;
   nInTimer::tInTimer_SC_Q_St_t state = nInTimer::kSC_Idle;     // Used to track the timing engine's state
   nInTimer::tInTimer_SC_Q_St_t prevState = nInTimer::kSC_Idle;

   // Trigger parameters
   tBoolean isTriggered = kFalse;

   // Buffer parameters
   const tBoolean allowOverwrite = kTrue; // Allow overwrite since we don't know when the reference trigger will assert
   tBoolean dataOverwritten = kFalse; // Has the DMA channel overwritten any unread data?
   u32 bytesRead = 0;      // Bytes read so far from the DMA buffer
   u32 bytesAvailable = 0; // Bytes in the DMA buffer
   const nNISTC3::tDMAChannelNumber dmaChannel = nNISTC3::kAI_DMAChannel;

   const u32 sampleSizeInBytes = sizeof(i16);
   const u32 totalNumberOfSamples = numberOfChannels * (postTrigSamps + preTrigSamps);
   const u32 dmaSizeInBytes = totalNumberOfSamples * sampleSizeInBytes;
   u32 count = 0; // How many samples have been taken?

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
   u32 SC_PreWaitCountTC_Error = 0;
   nInTimer::tInTimer_Error_t scanOverrun = nInTimer::kNO_ERROR;
   nInTimer::tInTimer_Error_t adcOverrun = nInTimer::kNO_ERROR;
   nInTimer::tInTimer_Error_t fifoOverflow = nInTimer::kNO_ERROR;
   nInTimer::tInTimer_Error_t SC_TC_Error = nInTimer::kNO_ERROR;
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

   //
   // Create subsystem helpers
   //

   nNISTC3::aiHelper aiHelper(device, deviceInfo->isSimultaneous, status);
   nNISTC3::eepromHelper eepromHelper(device, deviceInfo->isSimultaneous, deviceInfo->numberOfADCs, deviceInfo->numberOfDACs, status);
   nNISTC3::streamHelper streamHelper(device.AIStreamCircuit, device.CHInCh, status);

   //
   // Validate the Feature Parameters
   //

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

   // Ensure at least two post-trigger two scans
   if (postTrigSamps < 2)
   {
      printf("Error: Post-triggers samples (%u) must be 2 or greater.\n", postTrigSamps);
      status.setCode(kStatusBadParameter);
      return;
   }

   // Ensure at least two pre-trigger two scans
   if (preTrigSamps < 2)
   {
      printf("Error: Pre-trigger samples (%u) must be 2 or greater.\n", preTrigSamps);
      status.setCode(kStatusBadParameter);
      return;
   }

   // Coerce the the sample clock if it will overrun MIO devices
   if ( (!deviceInfo->isSimultaneous) && (samplePeriod < numberOfChannels*convertPeriod) )
   {
      printf("Warning: Coercing sample period from %u ticks to %u ticks to meet sample clock requirements.\n", samplePeriod, numberOfChannels*convertPeriod);
      samplePeriod = numberOfChannels*convertPeriod;
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

   // Program the START2 signal (reference trigger) to assert from an external line
   aiHelper.programStart2(
      refTrigSrc,                           // Set the line to be externally driven
      refTrigPolarity,                      // Make the line active on...
      kTrue,                                //   ...edge (not level)
      status);

   // Program the START signal (sample clock) to derive from the on-board clock
   aiHelper.programStart(
      nAI::kStartCnv_InternalTiming,        // Drive the clock line from internal oscillator
      nAI::kActive_High_Or_Rising_Edge,     // Make the line active on rising...
      kTrue,                                //   ...edge (not high level)
      status);

   if (deviceInfo->isSimultaneous)
   {
      // Program the convert to be the same as START
      aiHelper.programConvert(
         nAI::kStartCnv_InternalTiming,
         nAI::kActive_High_Or_Rising_Edge,
         status);
   }
   else
   {
      // Program the convert clock to start on the sample clock
      aiHelper.programConvert(
         nAI::kStartCnv_InternalTiming,        // Drive the clock line from internal sample clock
         nAI::kActive_Low_Or_Falling_Edge,     // Convert on falling edge
         status);
   }

   // Program the sample and convert clock timing specifications
   timingConfig.setAcqLevelTimingMode(nNISTC3::kInTimerPreTrigger, status); // This is a finite measurement with samples before the trigger (eg pre-trigger samples)
   timingConfig.setUseSICounter(kTrue, status); // Use SI for internal sample clocking
   timingConfig.setSamplePeriod(samplePeriod, status);
   timingConfig.setSampleDelay(sampleDelay, status);
   timingConfig.setNumberOfSamples(postTrigSamps, status); // Take postTrigSamps points after the trigger
   timingConfig.setStartTriggerHoldoffCount(preTrigSamps, status); // Take preTrigSamps points before the trigger
   if (!deviceInfo->isSimultaneous)
   {
      timingConfig.setUseSI2Counter(kTrue, status); // Use SI2 for internal convert clocking
      timingConfig.setConvertPeriod(convertPeriod, status);
      timingConfig.setConvertDelay(convertDelay, status);
   }

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

   dma->configure(bus, nNISTC3::kLinkChainRing, nNISTC3::kIn, dmaSizeInBytes, status);
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

   streamHelper.configureForInput(kFalse, dmaChannel, status);
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

   printf("Starting finite reference-triggered hardware-timed analog measurement.\n");
   aiHelper.getInTimerHelper(status).strobeStart1(status);

   /*********************************************************************\
   |
   |   Wait for the measurement to complete
   |
   \*********************************************************************/

   start = clock();
   state = device.AI.AI_Timer.Status_2_Register.readSC_Q_St(&status);
   prevState = state;
   while (state != nInTimer::kSC_Idle)
   {
      prevState = state;
      state = device.AI.AI_Timer.Status_2_Register.readSC_Q_St(&status);
      if (prevState != state)
      {
         printf("\n");
      }

      switch (state)
      {
      case nInTimer::kSC_PreCounting:
         count = device.AI.AI_Timer.SC_Save_Register.readRegister(&status);
         printf("%-63s: %6u\r", "Samples to read before the pretrigger sample requirement is met", count);
         break;
      case nInTimer::kSC_PreWait:
         printf("%-63s: %6.2f\r", "Seconds left to receive reference trigger", timeout-elapsedTime);
         break;
      case nInTimer::kSC_Counting:
         count = device.AI.AI_Timer.SC_Save_Register.readRegister(&status);
         printf("%-63s: %6u\r", "Samples to read before the posttrigger requirement is met", count);
         isTriggered = kTrue;
         continue;
      default:
         // Either the measurement completed or there was an error. Regardless,
         // the hardware has been disarmed. Break and check for errors.
         break;
      }

      if ((elapsedTime > timeout) && !isTriggered)
      {
         printf("\n");
         printf("Error: Reference trigger did not arrive within %.2f second timeout.\n", timeout);
         status.setCode(kStatusRLPTimeout);
         return;
      }
      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
   }

   /*********************************************************************\
   |
   |   Check for AI subsystem errors
   |
   \*********************************************************************/

   device.AI.AI_Timer.Status_1_Register.refresh(&status);
   SC_PreWaitCountTC_Error = device.AI.AI_Timer.Status_1_Register.getSC_PreWaitCountTC_ErrorSt(&status);
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
   SC_TC_Error = device.AI.AI_Timer.Status_1_Register.getSC_TC_Error_St(&status);

   if (SC_PreWaitCountTC_Error || scanOverrun || adcOverrun || fifoOverflow || SC_TC_Error)
   {
      aiErrored = kTrue;
   }

   /*********************************************************************\
   |
   |   Stop the voltage measurement
   |
   \*********************************************************************/

   // The AI subsystem has already been stopped and disarmed by hardware

   // Wait for the Stream Circuit to empty its FIFO
   rlpElapsedTime = 0;
   rlpStart = clock();
   while (!streamHelper.fifoIsEmpty(status))
   {
      // Spin on the FifoEmpty bit
      if (rlpElapsedTime > rlpTimeout)
      {
         printf("Error: Stream circuit did not flush within timeout.\n");
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

   nNISTC3::nAIDataHelper::printHeader(1, numberOfChannels, printVolts ? "Voltage" : "ADC code");

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
      // By allowing overwrites, the DMA buffer will skip to the newest data,
      // and since the buffer is exactly the size of the record (pre-trigger
      // samples + post-trigger samples), reading all of it at once returns
      // the entire record with the first pre-trigger sample as sample 1.
      dma->read(dmaSizeInBytes, reinterpret_cast<u8 *>(&rawData[0]), &bytesAvailable, allowOverwrite, &dataOverwritten, status);
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
   if (SC_PreWaitCountTC_Error)
   {
      printf("Error: SC reached terminal count twice while waiting for the reference trigger without acknowledgement.\n");
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
   if (SC_TC_Error)
   {
      printf("Error: SC reached terminal count twice without acknowledgement.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (! (aiErrored || dmaErrored))
   {
      printf("Finished finite reference-triggered hardware-timed analog measurement.\n");
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
