/*
 * aoex4.cpp
 *   Finite hardware-timed analog output with external reference clock
 *
 * aoex4 scales and generates finite analog data using hardware timing and
 * transfers data to the device using DMA. Before configuring the AO subsystem,
 * aoex4 programs the Phase-Lock Loop circuit to lock to an external reference
 * clock. After configuring the AO subsystem's timing and channels, aoex4
 * creates a sine wave to generate on the output channel and configures and
 * primes the DMA channel. Using the external reference clock as the timebase
 * for the on-board oscillator, aoex4 generates a finite number of analog
 * updates after it asserts the software start trigger. Finally, aoex4
 * restores the hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : voltage generation
 * ! Reference clock : external via PLL
 * ! Ref clock source: PFI0
 * ! Ref clock rate  : 10 MHz
 * Channel
 *   Channels        : ao0
 *   Scaling         : from Volts to raw DAC codes
 *   Output range    : +/- 10 V (*), +/- 5 V, APFI0, APFI1
 * Timing
 *   Sample mode     : finite
 *   Timing mode     : hardware-timed
 *   Clock source    : on-board oscillator
 *   Clock rate      : 50 kHz
 * Trigger
 *   Start trigger   : software
 *   Reference trig  : not supported
 *   Pause trigger   : none
 * Write Buffer
 *   Data transfer   : scatter-gather DMA
 *   DMA buffer      : wait for new data
 * Behavior
 *   Timeout         : 10 seconds
 *
 * External Connections
 *   ao0             : observe on an oscilloscope
 *   PFI0            : 10 MHz TTL reference clock
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
#include "pllHelper.h"
#include "outTimer/aoHelper.h"
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

   // Device parameters
   const nTriggers::tTrig_PLL_In_Source_Select_t desiredPLL_Source = nTriggers::kRefClkSrc_PFI0;

   // Channel parameters
   const u32 numberOfChannels = 1;                           // Channel: ao0
   tBoolean printVolts = kTrue;
   const nNISTC3::tOutputRange range = nNISTC3::kOutput_10V; // Range: +/- 10 Volts
   tBoolean resetToZero = kTrue;

   // Timing parameters
   const u32 samplePeriod = 2000; // Rate: 100 MHz / 2000 = 50 kHz

   // Buffer parameters
   u32 sampsPerChan = 512;
   const u32 dmaBufferFactor = 16;

   // Behavior parameters
   const f64 timeout = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Device parameters
   nNISTC3::PLL_Parameters_t parameters;
   parameters.frequency = nNISTC3::kReference10MHz;   // External reference frequency is assumed to be 10MHz
   parameters.pllDivisor = 1;                         // Divisor is always 1
   parameters.pllMultiplier =                         // Multiplier will multiply the refrence frequency...
      static_cast<u16>(100/parameters.frequency);     //   ...evenly to 100MHz
   parameters.pllOutputDivider = 8;                   // Output divider is always 8

   // Channel parameters
   u8 gain = 0;
   std::vector<nNISTC3::aoHelper::tChannelConfiguration> chanConfig(numberOfChannels, nNISTC3::aoHelper::tChannelConfiguration());

   // Timing parameters
   const nOutTimer::tOutTimer_Continuous_t operationType = nOutTimer::kFiniteOp;
   const u32 sampleDelay = 2; // Wait N TB3 ticks after the start trigger before clocking (N must be >= 2)

   // Trigger parameters
   const tBoolean triggerOnce = kTrue;

   // Buffer parameters
   const tBoolean allowRegeneration = kFalse;
   tBoolean dataRegenerated = kFalse; // Has the DMA channel regenerated old data?
   u32 availableSpace = 0; // Space in the DMA buffer
   const nNISTC3::tDMAChannelNumber dmaChannel = nNISTC3::kAO_DMAChannel;

   const u32 sampleSizeInBytes = sizeof(i16);
   const u32 totalNumberOfSamples = numberOfChannels * sampsPerChan;
   const u32 writeSizeInBytes = totalNumberOfSamples * sampleSizeInBytes;
   const u32 dmaSizeInBytes = dmaBufferFactor * writeSizeInBytes;

   std::vector<f32> voltData(sampsPerChan, 0);
   std::vector<f32> jointVoltData;
   std::vector<i16> scaledData(totalNumberOfSamples, 0);

   // Behavior parameters
   f64 elapsedTime = 0; // How long has the generation been running?
   clock_t start;
   const f64 rlpTimeout = 5; // Wait 5 seconds for a register operation before breaking
   clock_t rlpStart;
   f64 rlpElapsedTime;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tBoolean aoErrored = kFalse; // Did the AO subsystem have an error?
   nOutTimer::tOutTimer_Error_t BC_TCError = nOutTimer::kNo_error;
   nOutTimer::tOutTimer_Error_t fifoUnderflow = nOutTimer::kNo_error;
   nOutTimer::tOutTimer_Error_t dacOverrun = nOutTimer::kNo_error;
   nOutTimer::tOutTimer_Error_t BC_TCTrigError = nOutTimer::kNo_error;
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

   nNISTC3::pllHelper pllHelper(device, deviceInfo->isPCIe, status);
   nNISTC3::aoHelper aoHelper(device.AO, device.AO.AO_Timer, status);
   nNISTC3::eepromHelper eepromHelper(device, deviceInfo->isSimultaneous, deviceInfo->numberOfADCs, deviceInfo->numberOfDACs, status);
   nNISTC3::streamHelper streamHelper(device.AOStreamCircuit, device.CHInCh, status);

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

   // Ensure at least two updates
   if (sampsPerChan < 2)
   {
      printf("Error: Samples per channel (%u) must be 2 or greater.\n", sampsPerChan);
      status.setCode(kStatusBadParameter);
      return;
   }

   // Check STC3 Revision
   if (eepromHelper.getSTC3_Revision() == 1)
   {
      printf("Error: Unable to perform Phase-Loop Lock with STC3 Revision A.\n");
      status.setCode(kStatusBadParameter);
      return;
   }

   /*********************************************************************\
   |
   |   Program peripheral subsystems
   |
   \*********************************************************************/

   //
   // Configure and route PFI, RTSI, and trigger lines
   //

   // Set all PFI and RTSI lines to be input
   device.Triggers.PFI_Direction_Register.writeRegister(0x0, &status);
   device.Triggers.RTSI_Trig_Direction_Register.writeRegister(0x0, &status);

   //
   // Program external reference clock
   //

   pllHelper.enablePLL(parameters, desiredPLL_Source, status);
   if (status.isFatal())
   {
      return;
   }

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
   // Program AO timing
   //

   // Enter timing configuration mode
   device.AO.AO_Timer.Reset_Register.writeConfiguration_Start(kTrue, &status);

   // Disable external gating of the sample clock
   aoHelper.programExternalGate(
      nAO::kGate_Disabled,  // No external pause signal
      nAO::kRising_Edge,    // Don't care
      kFalse,               // Disable external gating
      status);

   // Program the START1 signal (start trigger) to assert from a software rising edge
   aoHelper.programStart1(
      nAO::kStart1_Pulse,   // Set the line to software-driven
      nAO::kRising_Edge,    // Make line active on rising...
      kTrue,                //   ...edge (not high level)
      status);

   // Program the START1 signal to be used immediately
   aoHelper.getOutTimerHelper(status).programStart1(
      nOutTimer::kSyncDefault,                // The trigger is synchronized to the source of the timer
      nOutTimer::kExportSynchronizedTriggers, // Exported triggers will be synchronized to the Update Interval source
      status);

   // Program the number of times that a buffer should be regenerated
   aoHelper.getOutTimerHelper(status).programBufferCount(
      dmaBufferFactor,      // Output dmaBufferFactor complete wave cycles
      status);

   // Program the number of samples in each buffer
   aoHelper.getOutTimerHelper(status).programUpdateCount(
      sampsPerChan,         // Each buffer will contain one complete wave cycle
      0,                    // Switching between different buffer sizes will not be used
      status);

   // Load the update count register
   aoHelper.getOutTimerHelper(status).loadUC(status);

   // Disable Buffer Counter gating since update source is UI_TC
   aoHelper.getOutTimerHelper(status).programBCGate(
      nOutTimer::kDisabled,
      status);

   // Program the Update signal (sample clock) to derive from the on-board clock
   aoHelper.programUpdate(
      nAO::kUpdate_UI_TC,   // Drive the clock line from the Update Interval Terminal Count
      nAO::kRising_Edge,    // Make the line active on rising edge
      status);

   // Program the Update Interval counter
   aoHelper.getOutTimerHelper(status).programUICounter(
      nOutTimer::kUI_Src_TB3,   // Derive the Update Interval from the internal timebase (which is now generated from the reference clock)
      nOutTimer::kRising_Edge,  // Make the line active on rising edge
      operationType,            // Finite operation
      status);

   // Set the number of clock AO UI clock intervals between updates
   aoHelper.getOutTimerHelper(status).loadUI(
      sampleDelay,          // Number of clock intervals after the start trigger before the first update
      samplePeriod,         // Number of clock intervals between successive updates
      status);

   // Set any errors that should stop the operation
   aoHelper.getOutTimerHelper(status).programStopCondition(
      triggerOnce,                // kTrue since this isn't a retriggerable operation
      operationType,              // Finite operation
      nOutTimer::kStop_on_Error,  // Stop if BC reaches its terminal count twice without acknowledgement
      nOutTimer::kStop_on_Error,  // Stop if a trigger arrives after BC reaches its terminal count
      nOutTimer::kStop_on_Error,  // Stop on Overrun_Error
      status);

   // Set the AO FIFO properties
   aoHelper.getOutTimerHelper(status).programFIFO(
      kTrue,                                // Enable the FIFO
      nOutTimer::kFifoMode_Less_Than_Full,  // Request data from DMA when FIFO is less than full
      nOutTimer::kDisabled,                 // Do not use on-board memory only
      status);

   // Clear the AO FIFO
   aoHelper.getOutTimerHelper(status).clearFIFO(status);

   //
   // Program AO channels
   //

   // Set the number of channels in this operation
   aoHelper.getOutTimerHelper(status).programNumberOfChannels(
      numberOfChannels,
      status);

   // Set the configuration for each channel
   for (u32 i=0; i<numberOfChannels; ++i)
   {
      chanConfig[i].channel = i; // aoN where N = i
      chanConfig[i].gain = gain; // Set the gain (used by the hardware)
      chanConfig[i].updateMode = nAO::kTimed; // Set the update mode
      chanConfig[i].range = range; // Set the range (used by the scaler)

      aoHelper.programConfigBank(
         chanConfig[i],    // Set the channel configuration for this channel
         status);
   }

   // Program the order that the channels appear in the data buffer (ao0 then ao1)
   aoHelper.programChannels(chanConfig, status);

   // Leave timing configuration mode
   device.AO.AO_Timer.Reset_Register.writeConfiguration_End(kTrue, &status);

   /*********************************************************************\
   |
   |   Program DMA
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

   // Create the sine wave
   nNISTC3::nAODataHelper::generateSignal(nNISTC3::nAODataHelper::kSineWave, -1, 1, voltData, sampsPerChan);

   // Replicate the sine wave for all requested channels
   nNISTC3::nAODataHelper::interleaveData(sampsPerChan, numberOfChannels, jointVoltData, voltData, voltData);

   // Scale the waveform
   nNISTC3::nAODataHelper::scaleData(jointVoltData, totalNumberOfSamples,
                                     scaledData, totalNumberOfSamples,
                                     chanConfig, eepromHelper);

   // Print the waveform
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

   for (u32 i=0; i<dmaBufferFactor; ++i)
   {
      dma->write(writeSizeInBytes, reinterpret_cast<u8 *>(&scaledData[0]), &availableSpace, allowRegeneration, &dataRegenerated, status);
      if (status.isFatal())
      {
         printf("Error: DMA write (%d).\n", status);
         return;
      }
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

   streamHelper.configureForOutput(kFalse, dmaChannel, status);
   streamHelper.enable(status);

   /*********************************************************************\
   |
   |   Start the voltage generation
   |
   \*********************************************************************/

   //
   // Wait for data to enter the FIFO
   //

   rlpElapsedTime = 0;
   rlpStart = clock();
   while (device.AO.AO_Timer.Status_1_Register.readFIFO_Empty_St(&status) == nOutTimer::kEmpty)
   {
      // Spin until the AO FIFO is no longer empty
      if (rlpElapsedTime > rlpTimeout)
      {
         printf("Error: AO FIFO did not receive data within timeout.\n");
         status.setCode(kStatusRLPTimeout);
         return;
      }
      rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
   }

   //
   // Arm the AO subsystem
   //

   // Send the first sample in the FIFO to each DAC to prevent glitching
   aoHelper.getOutTimerHelper(status).notAnUpdate(status);

   aoHelper.getOutTimerHelper(status).setArmUI(kTrue, status);
   aoHelper.getOutTimerHelper(status).setArmUC(kTrue, status);
   aoHelper.getOutTimerHelper(status).setArmBC(kTrue, status);
   aoHelper.getOutTimerHelper(status).armTiming(status);

   //
   // Start the AO subsystem
   //

   printf("Generating finite hardware-timed analog updates using external reference clock.\n");
   device.AO.AO_Timer.Command_1_Register.writeSTART1_Pulse(kTrue, &status);

   /*********************************************************************\
   |
   |   Wait for the generation to complete
   |
   \*********************************************************************/

   start = clock();
   while (! device.AO.AO_Timer.Status_1_Register.readBC_TC_St(&status))
   {
      // Check for AO subsystem errors
      device.AO.AO_Timer.Status_1_Register.refresh(&status);
      BC_TCError = device.AO.AO_Timer.Status_1_Register.getBC_TC_Error_St (&status);
      fifoUnderflow = device.AO.AO_Timer.Status_1_Register.getUnderflow_St(&status);
      dacOverrun = device.AO.AO_Timer.Status_1_Register.getOverrun_St(&status);
      BC_TCTrigError = device.AO.AO_Timer.Status_1_Register.getBC_TC_Trigger_Error_St (&status);

      if (BC_TCError || fifoUnderflow || dacOverrun || BC_TCTrigError)
      {
         aoErrored = kTrue;
         break;
      }

      // Spin until the Buffer Counter reaches the termination count
      if (elapsedTime > timeout)
      {
         printf("Error: Generation did not complete within %.2f second timeout.\n", timeout);
         status.setCode(kStatusRLPTimeout);
         return;
      }
      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
   }

   /*********************************************************************\
   |
   |   Stop the voltage generation
   |
   \*********************************************************************/

   //
   // Stop the AO subsystem
   //

   if (!aoErrored)
   {
      // Issue the stop command to end the operation when the update counter terminates
      device.AO.AO_Timer.Command_1_Register.writeEnd_On_UC_TC(kTrue, &status);

      // Wait for AO Operation to stop before disabling DMA
      rlpElapsedTime = 0;
      rlpStart = clock();
      while (device.AO.AO_Timer.Status_1_Register.readUC_Armed_St(&status) == nOutTimer::kArmed)
      {
         // Spin until the update counter reaches the terminal count and disarms or there is an error
         if (rlpElapsedTime > rlpTimeout)
         {
            printf("Error: AO timing engine did not stop within timeout.\n");
            status.setCode(kStatusRLPTimeout);
            return;
         }
         rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
      }
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

   if (BC_TCError)
   {
      printf("Error: BC reached terminal count twice without acknowledgement.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (fifoUnderflow)
   {
      printf("Error: AO FIFO underflow.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (dacOverrun)
   {
      printf("Error: DAC overrun.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (BC_TCTrigError)
   {
      printf("Error: Received a start trigger after BC reached terminal count without acknowledgement.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (!aoErrored)
   {
      printf("Finished generating finite hardware-timed analog updates using external reference clock.\n", sampsPerChan*dmaBufferFactor);
      printf("Generated %u %u-sample waveforms on %u channels (%u total samples) %s.\n",
         dmaBufferFactor,
         sampsPerChan,
         numberOfChannels,
         dmaBufferFactor*totalNumberOfSamples,
         dataRegenerated ? "by regenerating old data" : "without regenerating old data");
   }

   //
   // Restore the state of the device
   //

   // Nothing else to do: the helpers' destructors safely unwind device state.
}
