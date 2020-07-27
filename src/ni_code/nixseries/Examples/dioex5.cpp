/*
 * dioex5.cpp
 *   Continuous hardware-timed digital output with regeneration
 *
 * dioex5 generates a digital waveform using hardware timing and transfers data
 * to the device using DMA. After configuring the DO subsystem's timing
 * and channels, dioex5 creates simple incrementing waveform to generate on
 * port0 and configures and primes the DMA channel before sending a software
 * start trigger. For 10 seconds, data is regenerated from either the host
 * (using a circular DMA buffer) or the device (using its on-board FIFO)
 * until dioex5 stops the timing engine and shuts down DMA. Finally, dioex5
 * restores the hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : digital output
 * Channel
 *   Channels        : port0
 *   Scaling         : none
 *   Line mask       : all lines
 * ! Inversion mask  : no lines
 * Timing
 * ! Sample mode     : continuous regeneration
 *   Timing mode     : hardware-timed
 *   Clock source    : on-board oscillator
 *   Clock rate      : 1 kHz
 * Trigger
 *   Start trigger   : software
 *   Reference trig  : not supported
 *   Pause trigger   : none
 * Write Buffer
 * ! Data transfer   : DMA buffer
 * ! DMA transfer    : linear buffer for device regeneration
                       circular buffer for host regeneration
 *   DMA buffer      : wait for new data
 * ! Regeneration    : from on-board FIFO (*) or from host memory
 * Behavior
 *   Runtime         : 10 seconds
 *
 * External Connections
 *   port0           : observe with a logic analyzer
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
#include "dio/dioHelper.h"
#include "outTimer/doHelper.h"
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
   u32 lineMaskPort0 = 0xFFFFFFFF; // Use all of the lines in port0
   u32 invertMaskPort0 = 0x0; // Don't invert any lines in port0
   tBoolean tristateOnExit = kTrue;
   u32 defaultState = 0x0;

   // Timing parameters
   const u32 samplePeriod = 100000; // Rate: 100 MHz / 100e3 = 1 kHz

   // Buffer parameters
   tBoolean onboardRegen = kTrue;
   u32 sampsPerChan = 256; // With on-board regeneration, all samples on all channels must fit in the FIFO (2047 samples deep)

   // Behavior parameters
   const f64 runTime = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Channel parameters
   u32 port0Length = 0;

   // Timing parameters
   const nOutTimer::tOutTimer_Continuous_t operationType = nOutTimer::kContinuousOp;
   const u32 sampleDelay = 2; // Wait N TB3 ticks after the start trigger before clocking (N must be >= 2)

   // Trigger parameters
   const tBoolean triggerOnce = kTrue;

   // Buffer parameters
   const tBoolean allowRegeneration = kFalse;
   tBoolean dataRegenerated = kFalse; // Has the DMA channel regenerated old data?
   u32 bytesWritten = 0;   // Bytes written so far to the DMA buffer
   u32 availableSpace = 0; // Space in the DMA buffer
   const nNISTC3::tDMAChannelNumber dmaChannel = nNISTC3::kDO_DMAChannel;
   nOutTimer::tOutTimer_Disabled_Enabled_t regenMode;
   u32 dmaBufferFactor;
   nNISTC3::tDMATopology dmaTopology;
   u32 streamFifoSize = 0;

   if (onboardRegen)
   {
      regenMode = nOutTimer::kEnabled;
      // Use a single-use DMA buffer that holds the entire waveform once
      dmaBufferFactor = 1;
      dmaTopology = nNISTC3::kNormal;
   }
   else
   {
      regenMode = nOutTimer::kDisabled;
      // Use a ring DMA buffer that holds 16 waveforms
      dmaBufferFactor = 16;
      dmaTopology = nNISTC3::kLinkChainRing;
   }

   u32 sampleSizeInBytes;
   u32 writeSizeInBytes;
   u32 dmaSizeInBytes;
   nDO::tDO_DataWidth_t dataWidth;
   nDO::tDO_Data_Lane_t dataLane;

   // Using a vector as a self deleting byte buffer
   std::vector<u8> rawData;
   u8* rawData8 = NULL;
   u32* rawData32 = NULL;

   // Behavior parameters
   f64 elapsedTime = 0; // How long has the generation been running?
   clock_t start;
   const f64 rlpTimeout = 5; // Wait 5 seconds for a register operation before breaking
   clock_t rlpStart;
   f64 rlpElapsedTime;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tBoolean doErrored = kFalse; // Did the DO subsystem have an error?
   nOutTimer::tOutTimer_Error_t fifoUnderflow = nOutTimer::kNo_error;
   nOutTimer::tOutTimer_Error_t clkOverrun = nOutTimer::kNo_error;
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

   // Determine the size of Port 0
   port0Length = deviceInfo->port0Length;

   if (port0Length == 32)
   {
      // Port 0 has 32 lines for hardware-timed DIO
      lineMaskPort0 &= static_cast<u32>(nDI::nDI_FIFO_Data_Register::nCDI_FIFO_Data::kMask);
      invertMaskPort0 &= static_cast<u32>(nDI::nDI_FIFO_Data_Register::nCDI_FIFO_Data::kMask);
      dataWidth = nDO::kDO_FourBytes;
      dataLane = nDO::kDO_DataLane0;  // Doesn't matter with 4-byte data, ignored
      sampleSizeInBytes = 4;
   }
   else
   {
      // Port 0 has 8 lines for hardware-timed DIO
      lineMaskPort0 &= static_cast<u32>(nDI::nDI_FIFO_Data_Register8::nCDI_FIFO_Data8::kMask);
      invertMaskPort0 &= static_cast<u32>(nDI::nDI_FIFO_Data_Register8::nCDI_FIFO_Data8::kMask);
      dataWidth = nDO::kDO_OneByte;
      dataLane = nDO::kDO_DataLane0;
      sampleSizeInBytes = 1;
   }

   // Set the buffer parameters
   writeSizeInBytes = sampsPerChan * sampleSizeInBytes;
   dmaSizeInBytes = dmaBufferFactor * writeSizeInBytes;
   rawData.assign(writeSizeInBytes, 0);
   rawData8 = &rawData[0];
   rawData32 = reinterpret_cast<u32*>(&rawData[0]);

   //
   // Create subsystem helpers
   //

   nNISTC3::doHelper doHelper(device.DO, device.DO.DO_Timer, status);
   nNISTC3::dioHelper dioHelper(device.DI, device.DO, status);
   dioHelper.setTristate(tristateOnExit, status);
   nNISTC3::streamHelper streamHelper(device.DOStreamCircuit, device.CHInCh, status);

   //
   // Validate the Feature Parameters
   //

   // Ensure at least two updates
   if (sampsPerChan < 2)
   {
      printf("Error: Samples per channel (%u) must be 2 or greater.\n", sampsPerChan);
      status.setCode(kStatusBadParameter);
      return;
   }

   // Ensure that the FIFO can hold the waveform for on-board regeneration
   streamFifoSize = streamHelper.getFifoSize();
   if (onboardRegen && (writeSizeInBytes > streamFifoSize))
   {
      printf("Error: Number of waveform updates (%u) is greater than the DO FIFO size (%u).\n", writeSizeInBytes/sampleSizeInBytes, streamFifoSize/sampleSizeInBytes);
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
   |   Program the DO subsystem
   |
   \*********************************************************************/

   //
   // Reset the DO subsystem
   //

   doHelper.reset(status);
   dioHelper.reset(NULL, 0, status);

   // Prepare the default output state of the digital output lines
   // The default output state will not be output until configureLines is called
   device.DO.DO_DirectDataRegister.writeRegister(defaultState&lineMaskPort0, &status);
   device.DO.DO_Timer.Command_1_Register.writeRegister(nOutTimer::nUpdate_Pulse::kMask, &status);

   //
   // Program DO timing
   //

   // Enter timing configuration mode
   device.DO.DO_Timer.Reset_Register.writeConfiguration_Start(kTrue, &status);

   // Disable external gating of the sample clock
   doHelper.programExternalGate(
      nDO::kGate_Disabled,  // No external pause signal
      nDO::kRising_Edge,    // Don't care
      kFalse,               // Disable external gating
      status);

   // Program the START1 signal (start trigger) to assert from a software rising edge
   doHelper.programStart1(
      nDO::kStart1_Pulse,   // Set the line to software-driven
      nDO::kRising_Edge,    // Make line active on rising...
      kTrue,                //   ...edge (not high level)
      status);

   // Program the START1 signal to be used immediately
   doHelper.getOutTimerHelper(status).programStart1(
      nOutTimer::kSyncDefault,                // The trigger is synchronized to the source of the timer
      nOutTimer::kExportSynchronizedTriggers, // Exported triggers will be synchronized to the Update Interval source
      status);

   // Program the number of times that a buffer should be regenerated
   doHelper.getOutTimerHelper(status).programBufferCount(
      2,                    // Operation is continuous so the Buffer Counter is ignored
      status);

   // Program the number of samples in each buffer
   doHelper.getOutTimerHelper(status).programUpdateCount(
      sampsPerChan,         // Each buffer will contain a complete incrementing waveform
      0,                    // Switching between different buffer sizes will not be used
      status);

   // Load the update count register
   doHelper.getOutTimerHelper(status).loadUC(status);

   // Disable Buffer Counter gating
   doHelper.getOutTimerHelper(status).programBCGate(
      nOutTimer::kDisabled,
      status);

   // Program the Update signal (sample clock) to derive from the on-board clock
   doHelper.programUpdate(
      nDO::kUpdate_UI_TC,   // Drive the clock line from the Update Interval Terminal Count
      nDO::kRising_Edge,    // Make the line active on rising edge
      status);

   // Program the Update Interval counter
   doHelper.getOutTimerHelper(status).programUICounter(
      nOutTimer::kUI_Src_TB3,   // Derive the Update Interval from the internal timebase
      nOutTimer::kRising_Edge,  // Make the line active on rising edge
      operationType,            // Continuous operation
      status);

   // Set the number of clock DO UI clock intervals between updates
   doHelper.getOutTimerHelper(status).loadUI(
      sampleDelay,          // Number of clock intervals after the start trigger before the first update
      samplePeriod,         // Number of clock intervals between successive updates
      status);

   // Set any errors that should stop the operation
   doHelper.getOutTimerHelper(status).programStopCondition(
      triggerOnce,                    // kTrue since this is a continuous operation
      operationType,                  // Continuous operation
      nOutTimer::kContinue_on_Error,  // BC will reach its terminal count in a continuous operation, so don't stop when it does
      nOutTimer::kContinue_on_Error,  // Ignore additional start triggers
      nOutTimer::kStop_on_Error,      // Stop on Overrun_Error
      status);

   // Set the DO FIFO properties
   doHelper.getOutTimerHelper(status).programFIFO(
      kTrue,                                // Enable the FIFO
      nOutTimer::kFifoMode_Less_Than_Full,  // Request data from DMA when FIFO is less than full
      regenMode,                            // Use on-board memory only?
      status);

   // Set the FIFO layout
   device.DO.DO_Mode_Register.setDO_DataWidth(dataWidth, &status);
   device.DO.DO_Mode_Register.setDO_Data_Lane(dataLane, &status);
   device.DO.DO_Mode_Register.flush(&status);

   // Clear the DO FIFO
   doHelper.getOutTimerHelper(status).clearFIFO(status);

   //
   // Program DO channels
   //

   // Set the number of channels in this operation
   doHelper.getOutTimerHelper(status).programNumberOfChannels(
      1,       // Correlated digital output has only one channel -- port0
      status);

   dioHelper.configureLines(lineMaskPort0, nNISTC3::kCorrOutput, status);

   // Leave timing configuration mode
   device.DO.DO_Timer.Reset_Register.writeConfiguration_End(kTrue, &status);

   /*********************************************************************\
   |
   |   Program DMA
   |
   \*********************************************************************/

   //
   // Create and print output values
   //

   // Fill rawData with a simple incrementing waveform
   for (u32 j=0; j<sampsPerChan; ++j)
   {
      if (sampleSizeInBytes == 1)
      {
         rawData8[j] = static_cast<u8>(j ^ invertMaskPort0);
      }
      else
      {
         rawData32[j] = j ^ invertMaskPort0;
      }
   }

   nNISTC3::nDIODataHelper::printHeader(0);
   nNISTC3::nDIODataHelper::printData(rawData, writeSizeInBytes, sampleSizeInBytes);

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

   dma->configure(bus, dmaTopology, nNISTC3::kOut, dmaSizeInBytes, status);
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
      dma->write(writeSizeInBytes, &rawData[0], &availableSpace, allowRegeneration, &dataRegenerated, status);
      if (status.isFatal())
      {
         printf("Error: DMA write (%d).\n", status);
         return;
      }

      bytesWritten += writeSizeInBytes;
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

   streamHelper.configureForOutput(onboardRegen, dmaChannel, status);
   if (onboardRegen)
   {
      streamHelper.modifyTransferSize(dmaSizeInBytes, status);
   }
   streamHelper.enable(status);

   /*********************************************************************\
   |
   |   Start the digital generation
   |
   \*********************************************************************/

   //
   // Wait for data to enter the FIFO
   //

   if (onboardRegen)
   {
      // Wait for all data to be placed in the DO FIFO
      rlpElapsedTime = 0;
      rlpStart = clock();
      while (device.DO.DO_FIFO_St_Register.readRegister(&status) < dmaSizeInBytes/sampleSizeInBytes)
      {
         // Spin until the DO FIFO has all of the data
         if (rlpElapsedTime > rlpTimeout)
         {
            printf("Error: DO waveform did not load into FIFO within timeout.\n");
            status.setCode(kStatusRLPTimeout);
            return;
         }
         rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
      }
   }
   else
   {
      // Wait for data to enter the DO FIFO
      rlpElapsedTime = 0;
      rlpStart = clock();
      while (device.DO.DO_Timer.Status_1_Register.readFIFO_Empty_St(&status) == nOutTimer::kEmpty)
      {
         // Spin until the DO FIFO is no longer empty
         if (rlpElapsedTime > rlpTimeout)
         {
            printf("Error: DO FIFO did not receive data within timeout.\n");
            status.setCode(kStatusRLPTimeout);
            return;
         }
         rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
      }
   }

   //
   // Arm the DO subsystem
   //

   doHelper.getOutTimerHelper(status).setArmUI(kTrue, status);
   doHelper.getOutTimerHelper(status).setArmUC(kTrue, status);
   doHelper.getOutTimerHelper(status).setArmBC(kTrue, status);
   doHelper.getOutTimerHelper(status).armTiming(status);

   //
   // Start the DO subsystem
   //

   printf("Generating continuous hardware-timed digital updates from the %s for %.2f seconds.\n", onboardRegen ? "device" : "host", runTime);
   device.DO.DO_Timer.Command_1_Register.writeSTART1_Pulse(kTrue, &status);

   /*********************************************************************\
   |
   |   Wait for the generation to complete
   |
   \*********************************************************************/

   start = clock();
   while (elapsedTime < runTime)
   {
      // Check for DO subsystem errors
      device.DO.DO_Timer.Status_1_Register.refresh(&status);
      fifoUnderflow = device.DO.DO_Timer.Status_1_Register.getUnderflow_St(&status);
      clkOverrun = device.DO.DO_Timer.Status_1_Register.getOverrun_St(&status);

      if (fifoUnderflow || clkOverrun)
      {
         doErrored = kTrue;
         break;
      }

      // Wait for the run time to expire.
      //   For on-board regeneration, the FIFO will continually loop.
      //   For host regeneration, the DMA buffer will continually loop.
      // In either case, further writes are not necessary.
      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
   }

   /*********************************************************************\
   |
   |   Stop the digital generation
   |
   \*********************************************************************/

   //
   // Stop the DO subsystem
   //

   if (!doErrored)
   {
      // Issue the stop command to end the operation when the update counter terminates
      device.DO.DO_Timer.Command_1_Register.writeEnd_On_UC_TC(kTrue, &status);

      // Wait for DO Operation to stop before disabling DMA
      rlpElapsedTime = 0;
      rlpStart = clock();
      while (device.DO.DO_Timer.Status_1_Register.readUC_Armed_St(&status) == nOutTimer::kArmed)
      {
         // Spin until the update counter reaches the terminal count and disarms or there is an error
         if (rlpElapsedTime > rlpTimeout)
         {
            printf("Error: DO timing engine did not stop within timeout.\n");
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

   if (fifoUnderflow)
   {
      printf("Error: DO FIFO underflow.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (clkOverrun)
   {
      printf("Error: Sample clock overrun.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (!doErrored)
   {
      printf("Finished generating continuous hardware-timed digital updates from the %s for %.2f seconds.\n", onboardRegen ? "device" : "host", runTime);
      printf("Generated %u %u-sample waveforms (%u total samples) %s.\n",
         bytesWritten/(sampsPerChan*sampleSizeInBytes),
         sampsPerChan,
         bytesWritten/sampleSizeInBytes,
         dataRegenerated ? "by regenerating old data" : "without regenerating old data");
   }

   //
   // Restore the state of the device
   //

   // Nothing else to do: the helpers' destructors safely unwind device state.
}
