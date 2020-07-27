/*
 * aiex1.cpp
 *   Single-point on-demand analog input
 *
 * aiex1 reads and scales analog data using software timing and transfers data
 * to the host by reading directly from the FIFO. After configuring the AI
 * subsystem's timing and channel parameters, aiex1 initiates a configurable
 * number of scans and reads, scales and prints the data. Finally, aiex1
 * restores the hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : voltage measurement
 * Channel
 * ! Channels        : ai0, ai1, ai2, ai3
 * ! Scaling         : Volts (*) or raw ADC codes
 *   Terminal config : RSE
 * ! Input range     : +/- 10 V (*),  +/- 5 V,     +/- 2 V,    +/- 1 V,
 *                     +/- 500mV,     +/- 200 mV,  +/- 100 mV
 * Timing
 * ! Sample mode     : single-point
 * ! Timing mode     : software-timed
 * ! Clock source    : software strobe
 * Trigger
 * ! Start trigger   : automatic start
 *   Reference trig  : none
 *   Pause trigger   : none
 * Read Buffer
 * ! Data transfer   : programmed IO from FIFO
 *
 * External Connections
 *   ai0:3           : voltages within +/- 10 V (*) or other specified range
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
   const u32 numberOfChannels = 4;                          // Channels: ai0:3
   tBoolean printVolts = kTrue;
   const nNISTC3::tInputRange range = nNISTC3::kInput_10V;  // Range: +/- 10 Volts

   // Buffer parameters
   const u32 sampsPerChan = 25;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Channel parameters
   u16 gain = 0;
   std::vector<nNISTC3::aiHelper::tChannelConfiguration> chanConfig(numberOfChannels, nNISTC3::aiHelper::tChannelConfiguration());
   u32 maxChannel = numberOfChannels;

   // Timing parameters
   const u32 convertPeriod = 400; // 100 MHz / 400 = 250 kHz (refer to device specs for minimum period)
   const u32 convertDelay = 2; // Wait N TB3 ticks after the sample clock before converting (N must be >= 2)
   nNISTC3::inTimerParams timingConfig;

   // Buffer parameters
   u32 n = 0;  // Number of samples counter
   u32 m = 0;  // Number of channels counter
   u32 samplesAvailable = 0; // Number of samples in the AI data FIFO
   u32 samplesRead = 0; // Number of samples read from the AI data FIFO
   u32 streamFifoSize = 0;

   const u32 sampleSizeInBytes = sizeof(i16);
   const u32 readSizeInBytes = numberOfChannels * sampleSizeInBytes;

   std::vector<i16> rawData(numberOfChannels, 0);
   std::vector<f32> scaledData(numberOfChannels, 0);

   // Behavior parameters
   const f64 rlpTimeout = 5; // Wait 5 seconds for a register operation before breaking
   clock_t rlpStart;
   f64 rlpElapsedTime;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tBoolean aiErrored = kFalse; // Did the AI subsystem have an error?
   nInTimer::tInTimer_Error_t scanOverrun = nInTimer::kNO_ERROR;
   nInTimer::tInTimer_Error_t adcOverrun = nInTimer::kNO_ERROR;
   nInTimer::tInTimer_Error_t fifoOverflow = nInTimer::kNO_ERROR;
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

   // Ensure that the FIFO can hold a complete measurement
   streamFifoSize = streamHelper.getFifoSize();
   if (readSizeInBytes > streamFifoSize)
   {
      printf("Error: Number of samples (%u) is greater than the AI FIFO size (%u).\n", numberOfChannels, streamFifoSize/sampleSizeInBytes);
      status.setCode(kStatusBadParameter);
      return;
   }

   /*********************************************************************\
   |
   |   Program peripheral subsystems
   |
   \*********************************************************************/

   // No peripheral subsystems used in this example

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

   // Auto-trigger the START1 signal (start trigger)
   aiHelper.programStart1(
      nAI::kStart1_Low,                     // Drive trigger line low
      nAI::kActive_Low_Or_Falling_Edge,     // Make line active on low...
      kFalse,                               //   ...level (not falling edge)
      status);

   // Program the START signal (sample clock) to start on a software strobe
   aiHelper.programStart(
      nAI::kStartCnv_Low,                   // Drive the clock line low
      nAI::kActive_High_Or_Rising_Edge,     // Make the line active on rising...
      kTrue,                                //   ...edge (not high level)
      status);

   if (deviceInfo->isSimultaneous)
   {
      // Program the convert to be the same as START
      aiHelper.programConvert(
         nAI::kStartCnv_Low,
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
      chanConfig[i].channelType = nAI::kRSE;        // Single-ended: aiN vs aiGnd
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

   printf("Starting %u-sample on-demand analog measurement.\n", sampsPerChan);
   aiHelper.getInTimerHelper(status).armTiming(timingConfig, status);

   // The START1 signal (start trigger) auto-triggers

   /*********************************************************************\
   |
   |   Read and print data
   |
   \*********************************************************************/

   nNISTC3::nAIDataHelper::printHeader(1, numberOfChannels, printVolts ? "Voltage" : "ADC code");

   // Perform an on-demand read of each channel and scale the data
   for (n=0; n<sampsPerChan; ++n)
   {
      // Begin a scan
      if (deviceInfo->isSimultaneous)
      {
         aiHelper.getInTimerHelper(status).strobeConvert(status);
      }
      else
      {
         aiHelper.getInTimerHelper(status).strobeStart(status);
      }

      // Wait for the scan to complete
      rlpElapsedTime = 0;
      rlpStart = clock();
      while (device.AI.AI_Timer.Status_1_Register.readScan_In_Progress_St(&status))
      {
         // Spin on the Scan In Progress bit
         if (rlpElapsedTime > rlpTimeout)
         {
            printf("\n");
            printf("Error: AI scan did not complete within timeout.");
            status.setCode(kStatusRLPTimeout);
            break;
         }
         rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
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

      // Note: these errors are unlikely to occur in this example because the
      // data is read between each sample clock pulse. However, if the hardware
      // were programmed another way, they may happen.
      if (scanOverrun || adcOverrun || fifoOverflow)
      {
         aiErrored = kTrue;
         break;
      }

      // Make sure there is data in the FIFO before reading from it
      rlpElapsedTime = 0;
      rlpStart = clock();
      while (rlpElapsedTime < rlpTimeout)
      {
         samplesAvailable = device.AI.AI_Data_FIFO_Status_Register.readRegister(&status);

         if (samplesAvailable >= numberOfChannels)
         {
            // Read from the 16-bit FIFO data register since it was configured as such
            for (m=0; m<numberOfChannels; ++m)
            {
               rawData[m] = device.AI.AI_FIFO_Data_Register16.readRegister();
            }

            // Print the data
            if (printVolts)
            {
               nNISTC3::nAIDataHelper::scaleData(rawData, numberOfChannels,
                                                 scaledData, numberOfChannels,
                                                 chanConfig, eepromHelper, *deviceInfo);
               nNISTC3::nAIDataHelper::printData(scaledData, numberOfChannels, numberOfChannels);
            }
            else
            {
               nNISTC3::nAIDataHelper::printData(rawData, numberOfChannels, numberOfChannels);
            }
            samplesRead += numberOfChannels;
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
         printf("Error: AI FIFO expected %u samples, only %u available.", numberOfChannels, samplesAvailable);
         status.setCode(kStatusRLPTimeout);
         break;
      }
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
   if (!aiErrored)
   {
      printf("Finished on-demand analog measurement.\n");
      printf("Read %u samples on %u channels (%u total samples).\n",
             sampsPerChan,
             numberOfChannels,
             samplesRead);
   }

   //
   // Restore the state of the device
   //

   // Nothing else to do: the helpers' destructors safely unwind device state.
}
