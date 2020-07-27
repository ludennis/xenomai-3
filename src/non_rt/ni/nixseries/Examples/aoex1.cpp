/*
 * aoex1.cpp
 *   Single-point on-demand analog output
 *
 * aoex1 scales and generates analog data using software timing and transfers
 * data to the device by writing directly to the DAC data registers. After
 * configuring the AO subsystem's channels, aoex1 creates ramp waveforms to
 * generate on each channel. Since aoex1 writes directly to the DAC's data
 * registers, it does not need to configure the timing engine or FIFO. After
 * generating the waveforms, aoex1 restores the hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : voltage generation
 * Channel
 * ! Channels        : ao0, ao1
 * ! Scaling         : from Volts to raw DAC codes
 * ! Output range    : +/- 10 V (*), +/- 5 V, APFI0, APFI1
 * Timing
 * ! Sample mode     : single-point
 * ! Timing mode     : software-timed
 *   Clock source    : software timer
 *   Clock rate      : 100 Hz
 * Trigger
 *   Start trigger   : none
 *   Reference trig  : not supported
 *   Pause trigger   : none
 * Write Buffer
 * ! Data transfer   : programmed IO to registers
 *
 * External Connections
 *   ao0:1           : observe on an oscilloscope
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
#include "outTimer/aoHelper.h"
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
   const u32 numberOfChannels = 2;                           // Channels: ao0:1
   tBoolean printVolts = kTrue;
   const nNISTC3::tOutputRange range = nNISTC3::kOutput_10V; // Range: +/- 10 Volts
   tBoolean resetToZero = kTrue;

   // Timing parameters
   const f64 softwareUpdateInterval = .01; // Rate: 100 Hz

   // Buffer parameters
   const u32 sampsPerChan = 20;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Channel parameters
   u8 gain = 0;
   std::vector<nNISTC3::aoHelper::tChannelConfiguration> chanConfig(numberOfChannels, nNISTC3::aoHelper::tChannelConfiguration());

   // Buffer parameters
   u32 n = 0;  // Number of samples counter
   u32 m = 0;  // Number of channels counter

   const u32 totalNumberOfSamples = numberOfChannels * sampsPerChan;

   std::vector<f32> voltData1(sampsPerChan, 0);
   std::vector<f32> voltData2(sampsPerChan, 0);
   std::vector<f32> jointVoltData;
   std::vector<i16> scaledData(totalNumberOfSamples, 0);

   // Behavior parameters
   f64 elapsedTime = 0; // How long has the generation been running?
   clock_t start;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tBoolean aoErrored = kFalse; // Did the AO subsystem have an error?
   nOutTimer::tOutTimer_Error_t dacOverrun = nOutTimer::kNo_error;
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

   //
   // Create subsystem helpers
   //

   nNISTC3::aoHelper aoHelper(device.AO, device.AO.AO_Timer, status);
   nNISTC3::eepromHelper eepromHelper(device, deviceInfo->isSimultaneous, deviceInfo->numberOfADCs, deviceInfo->numberOfDACs, status);

   //
   // Validate the Feature Parameters
   //

   // Ensure that the number of DACs match the number of channels requested
   if (numberOfChannels > deviceInfo->numberOfDACs)
   {
      printf("Error: Number of channels requested (%u) is greater than number of DACs on the device (%u).\n",
         numberOfChannels, deviceInfo->numberOfDACs);
      status.setCode(kStatusBadParameter);
      return;
   }

   // Ensure that the range is supported by this board
   gain = deviceInfo->getAO_Gain(range, status);
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

   // No peripheral subsystems used in this example

   /*********************************************************************\
   |
   |   Program the AO subsystem
   |
   \*********************************************************************/

   //
   // Reset the AO subsystem
   //

   aoHelper.reset(status);

   //
   // Program AO channels
   //

   // Enter timing configuration mode
   device.AO.AO_Timer.Reset_Register.writeConfiguration_Start(kTrue, &status);

   // Set the configuration for each channel
   for (u32 i=0; i<numberOfChannels; ++i)
   {
      chanConfig[i].channel = i; // aoN where N = i
      chanConfig[i].gain = gain; // Set the gain (used by the hardware)
      chanConfig[i].updateMode = nAO::kImmediate; // Set the update mode
      chanConfig[i].range = range; // Set the range (used by the scaler)

      aoHelper.programConfigBank(
         chanConfig[i],    // Set the channel configuration for this channel
         status);
   }

   // Leave timing configuration mode
   device.AO.AO_Timer.Reset_Register.writeConfiguration_End(kTrue, &status);

   /*********************************************************************\
   |
   |   Program output data
   |
   \*********************************************************************/

   //
   // Create, scale, and print output voltages
   //

   // Create zero voltage for each ao channel
   std::vector<f32> zeroVolts(numberOfChannels, 0);
   std::vector<i16> scaledZeroVolts(numberOfChannels, 0);
   nNISTC3::nAODataHelper::scaleData(zeroVolts, numberOfChannels,
      scaledZeroVolts, numberOfChannels,
      chanConfig, eepromHelper);

   // Give aoHelper enough information to set output to 0V in the event of an error
   aoHelper.setZeroVolts(scaledZeroVolts, resetToZero, status);

   // Create the ramp for ao0
   nNISTC3::nAODataHelper::generateSignal(nNISTC3::nAODataHelper::kRamp, -5, 5, voltData1, sampsPerChan);

   // Create the ramp for ao1
   nNISTC3::nAODataHelper::generateSignal(nNISTC3::nAODataHelper::kRamp, 5, -5, voltData2, sampsPerChan);

   // Interleave both channels' data into a single buffer (ao0#0, ao1#0; ao0#1, ao1#1, ...)
   nNISTC3::nAODataHelper::interleaveData(sampsPerChan, numberOfChannels, jointVoltData, voltData1, voltData2);

   // Scale both waveforms
   nNISTC3::nAODataHelper::scaleData(jointVoltData, totalNumberOfSamples,
                                     scaledData, totalNumberOfSamples,
                                     chanConfig, eepromHelper);

   // Print both waveforms
   nNISTC3::nAODataHelper::printHeader(1, numberOfChannels, printVolts ? "Voltage" : "DAC code");
   if (printVolts)
   {
      nNISTC3::nAODataHelper::printData(jointVoltData, totalNumberOfSamples, numberOfChannels);
   }
   else
   {
      nNISTC3::nAODataHelper::printData(scaledData, totalNumberOfSamples, numberOfChannels);
   }
   printf("\n");

   /*********************************************************************\
   |
   |   Write data
   |
   \*********************************************************************/

   printf("Generating software-timed analog updates.\n");
   for (n=0; n<totalNumberOfSamples; n+=numberOfChannels)
   {
      start = clock();

      // Update the channels individually
      for (m=0; m<numberOfChannels; ++m)
      {
         device.AO.AO_Direct_Data[m].writeRegister(scaledData[n+m], &status);
      }

      // Check for AO subsystem errors
      device.AO.AO_Timer.Status_1_Register.refresh(&status);
      dacOverrun = device.AO.AO_Timer.Status_1_Register.getOverrun_St(&status);

      // Note: this error is unlikely to occur in this example because the
      // data is written between each update.
      if (dacOverrun)
      {
         aoErrored = kTrue;
         break;
      }

      // Use software timing to delay the updates
      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
      while (elapsedTime < softwareUpdateInterval)
      {
         elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
      }
   }

   /*********************************************************************\
   |
   |   Finalize the generation
   |
   \*********************************************************************/

   //
   // Print run-time summary
   //

   if (dacOverrun)
   {
      printf("Error: DAC overrun.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (!aoErrored)
   {
      printf("Finished generating %u software-timed analog updates on %u channels (%u total samples).\n",
         sampsPerChan,
         numberOfChannels,
         totalNumberOfSamples);
   }

   //
   // Restore the state of the device
   //

   // Nothing else to do: the helpers' destructors safely unwind device state.
}
