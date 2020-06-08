/*
 * aiex2.cpp
 *   Retriggerable finite hardware-timed analog input
 *
 * aiex2 reads and scales analog data using hardware timing and transfers data
 * to the host by reading directly from the FIFO. After configuring the start
 * trigger input and the AI subsystem's timing and channel parameters, aiex2
 * arms the timing engine. For 10 seconds, data is read, scaled, and printed
 * in chunks on each start trigger received until aiex2 disarms the timing
 * engine. Finally, aiex2 restores the hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : voltage measurement
 * Channel
 *   Channels        : ai0, ai1, ai2, ai3
 *   Scaling         : Volts (*) or raw ADC codes
 * ! Terminal config : RSE (*), differential, non-referenced single-ended
 *   Input range     : +/- 10 V (*),  +/- 5 V,     +/- 2 V,    +/- 1 V,
 *                     +/- 500mV,     +/- 200 mV,  +/- 100 mV
 * Timing
 * ! Sample mode     : finite
 * ! Timing mode     : hardware-timed
 * ! Clock source    : on-board oscillator
 * ! Clock rate      : 20 kHz sample clock; 200 kHz convert clock (MIO only)
 * Trigger
 * ! Start trigger   : PFI0 (retriggerable digital rising edge)
 *   Reference trig  : none
 *   Pause trigger   : none
 * Read Buffer
 *   Data transfer   : programmed IO from FIFO
 * Behavior
 *   Timeout         : 10 seconds
 *
 * External Connections
 *   ai0:3           : voltages within +/- 10 V (*) or other specified range
 *   PFI0            : TTL start trigger
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
#include "dataHelper.h"
#include "devices.h"
#include "eepromHelper.h"
#include "inTimer/aiHelper.h"
#include "simultaneousInit.h"
#include "streamHelper.h"

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
   u32 samplePeriod = 5000; // 100 MHz / 5000 = 20 kHz

   // Trigger parameters
   const nAI::tAI_START1_Select_t startTrigger = nAI::kStart1_PFI0;
   const nAI::tAI_Polarity_t startTrigPolarity = nAI::kActive_High_Or_Rising_Edge; // Use rising edge

   // Buffer parameters
   u32 sampsPerChan = 25;

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
   const u32 sampleDelay = 2; // Wait N TB3 ticks after the start trigger before clocking (N must be >= 2)
   const u32 convertPeriod = 400; // 100 MHz / 400 = 250 kHz (refer to device specs for minimum period)
   const u32 convertDelay = 2; // Wait N TB3 ticks after the sample clock before converting (N must be >= 2)
   nNISTC3::inTimerParams timingConfig;

   // Trigger parameters
   u32 triggersReceived = 0;

   // Buffer parameters
   u32 samplesAvailable = 0; // Number of samples in the AI data FIFO
   u32 samplesRead = 0; // Number of samples read from the AI data FIFO
   u32 streamFifoSize = 0;

   const u32 sampleSizeInBytes = sizeof(i16);
   const u32 totalNumberOfSamples = numberOfChannels * sampsPerChan;
   const u32 readSizeInBytes = totalNumberOfSamples * sampleSizeInBytes;

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
   nInTimer::tInTimer_Error_t SC_TC_Error = nInTimer::kNO_ERROR;
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

   // Ensure at least two scans
   if (sampsPerChan < 2)
   {
      printf("Error: Samples per channel (%u) must be 2 or greater.\n", sampsPerChan);
      status.setCode(kStatusBadParameter);
      return;
   }

   // Ensure that the FIFO can hold a complete measurement
   streamFifoSize = streamHelper.getFifoSize();
   if (readSizeInBytes > streamFifoSize)
   {
      printf("Error: Number of samples (%u) is greater than the AI FIFO size (%u).\n", totalNumberOfSamples, streamFifoSize/sampleSizeInBytes);
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

   // Program the START1 signal (start trigger) to assert from an external signal
   aiHelper.programStart1(
      startTrigger,                         // Set the PFI line
      startTrigPolarity,                    // Make line active on...
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
   timingConfig.setAcqLevelTimingMode(nNISTC3::kInTimerPostTrigger, status); // This is a finite measurement with samples after the trigger (eg post-trigger samples)
   timingConfig.setUseSICounter(kTrue, status); // Use SI for internal sample clocking
   timingConfig.setSamplePeriod(samplePeriod, status);
   timingConfig.setSampleDelay(sampleDelay, status);
   timingConfig.setNumberOfSamples(sampsPerChan, status);
   timingConfig.setRetriggerRecord(kTrue, status);   // Set the measurement to be retriggerable
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
   |   Start the voltage measurement
   |
   \*********************************************************************/

   //
   // Arm the AI subsystem
   //

   printf("Waiting %.2f seconds for start triggers.\n", runTime);
   aiHelper.getInTimerHelper(status).armTiming(timingConfig, status);

   /*********************************************************************\
   |
   |   Read and print data
   |
   \*********************************************************************/

   nNISTC3::nAIDataHelper::printHeader(1, numberOfChannels, printVolts ? "Voltage" : "ADC code");

   start = clock();
   while (elapsedTime < runTime)
   {
      // Determine if a measurement has started
      if (device.AI.AI_Timer.Status_2_Register.readSC_Q_St(&status) == nInTimer::kSC_Counting)
      {
         ++triggersReceived;

         // If so, wait for the measurement to complete
         rlpElapsedTime = 0;
         rlpStart = clock();
         while (!device.AI.AI_Timer.Status_1_Register.readSC_TC_St(&status))
         {
            // Spin on the Scan Counter Terminal Count Status
            if (rlpElapsedTime > rlpTimeout)
            {
               printf("\n");
               printf("Error: AI scan did not complete within timeout.");
               status.setCode(kStatusRLPTimeout);
               break;
            }
            rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
         }

         // Make sure there is data in the FIFO before reading from it
         rlpElapsedTime = 0;
         rlpStart = clock();
         while (rlpElapsedTime < rlpTimeout)
         {
            samplesAvailable = device.AI.AI_Data_FIFO_Status_Register.readRegister(&status);

            if (samplesAvailable >= totalNumberOfSamples)
            {
               // Read from the 16-bit FIFO data register since it was configured as such
               for (u32 i=0; i<totalNumberOfSamples; ++i)
               {
                  rawData[i] = device.AI.AI_FIFO_Data_Register16.readRegister();
               }

               // Print the data
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
               samplesRead += totalNumberOfSamples;

               // Acknowledge the START1 trigger event and SC_TC event
               device.AI.AI_Timer.Interrupt1_Register.writeSTART1_Interrupt_Ack(kTrue, &status);
               device.AI.AI_Timer.Interrupt1_Register.writeSC_TC_Interrupt_Ack(kTrue, &status);
               break;
            }
            else
            {
               rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
            }
         }

         if (rlpElapsedTime > rlpTimeout)
         {
            printf("\n");
            printf("Error: AI FIFO expected %u samples, only %u available.", totalNumberOfSamples, samplesAvailable);
            status.setCode(kStatusRLPTimeout);
            break;
         }
      }

      // Check for AI subsystem errors
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
      SC_TC_Error = device.AI.AI_Timer.Status_1_Register.getSC_TC_Error_St(&status);

      if (scanOverrun || adcOverrun || fifoOverflow || SC_TC_Error)
      {
         aiErrored = kTrue;
         break;
      }

      // Update the run time for this measurement
      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
   }
   printf("\n");

   /*********************************************************************\
   |
   |   Stop the voltage measurement
   |
   \*********************************************************************/

   // The AI subsystem has already been stopped by hardware

   device.AI.AI_Timer.Command_Register.writeDisarm(kTrue, &status);

   // Wait for the timing engine to disarm
   rlpElapsedTime = 0;
   rlpStart = clock();
   while (device.AI.AI_Timer.Status_1_Register.readSC_Armed_St(&status) == nInTimer::kArmed)
   {
      // Spin on the SC Armed bit
      if (rlpElapsedTime > rlpTimeout)
      {
         printf("\n");
         printf("Error: AI timing engine did not disarm within timeout.\n");
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
   if (fifoOverflow)
   {
      printf("Error: AI FIFO overflow.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (SC_TC_Error)
   {
      printf("Error: SC reached terminal count twice without acknowledgement.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (!aiErrored)
   {
      printf("Finished retriggerable finite hardware-timed analog measurement.\n");
      printf("Received %u triggers, reading %u samples on %u channels (%u total samples).\n",
             triggersReceived,
             samplesRead/numberOfChannels,
             numberOfChannels,
             samplesRead);
   }

   //
   // Restore the state of the device
   //

   // Nothing else to do: the helpers' destructors safely unwind device state.
}
