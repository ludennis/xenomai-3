/*
 * dioex4.cpp
 *   Continuous hardware-timed digital input with optimized DMA
 *
 * dioex4 reads a digital waveform using hardware timing and transfers data
 * to the host using an optimized DMA buffer. After configuring the DI
 * subsystem's timing and channel parameters, dioex4 configures and starts the
 * DMA channel before sending a software start trigger. For 10 seconds, data is
 * read and printed in chunks until dioex4 stops the timing engine, shuts
 * down DMA, and flushes the buffer. Finally, dioex4 restores the hardware's
 * previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : digital input
 * Channel
 *   Channels        : port0
 *   Scaling         : none
 *   Line mask       : all lines
 * Timing
 * ! Sample mode     : continuous
 *   Timing mode     : hardware-timed
 *   Clock source    : on-board oscillator
 *   Clock rate      : 10 kHz
 * Trigger
 *   Start trigger   : software
 *   Reference trig  : none
 *   Pause trigger   : none
 * Read Buffer
 *   Data transfer   : optimized scatter-gather DMA ring
 * ! DMA transfer    : the buffer is optimized to maximize throughput
 * ! DMA buffer      : overwrite or preserve (*) unread samples
 * Behavior
 *   Runtime         : 10 seconds
 *
 * External Connections
 *   port0           : TTL signals
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
#include "inTimer/diHelper.h"
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

   // Timing parameters
   const u32 samplePeriod = 10000; // Rate: 100 MHz / 10e3 = 10 kHz
   const u32 sampleDelay = 2;      // Wait N TB3 ticks after the start trigger before clocking (N must be >= 2)

   // Buffer parameters
   const u32 sampsPerChan = 1024;
   const u32 dmaBufferFactor = 16;
   tBoolean allowOverwrite = kFalse;

   // Behavior parameters
   const f64 runTime = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Channel parameters
   u32 port0Length = 0;

   // Timing parameters
   nNISTC3::inTimerParams timingConfig;

   // Buffer parameters
   tBoolean dataOverwritten = kFalse; // Has the DMA channel overwritten any unread data?
   u32 bytesRead = 0;      // Bytes read so far from the DMA buffer
   u32 bytesAvailable = 0; // Bytes in the DMA buffer
   const nNISTC3::tDMAChannelNumber dmaChannel = nNISTC3::kDI_DMAChannel;

   u32 sampleSizeInBytes;
   u32 readSizeInBytes;
   u32 dmaSizeInBytes;
   nDI::tDI_DataWidth_t dataWidth;
   nDI::tDI_Data_Lane_t dataLane;

   // Using a vector as a self deleting byte buffer
   std::vector<u8> rawData;

   // Behavior parameters
   f64 elapsedTime = 0; // How long has the measurement been running?
   clock_t start;
   const f64 rlpTimeout = 5; // Wait 5 seconds for a register operation before breaking
   clock_t rlpStart;
   f64 rlpElapsedTime;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tBoolean diErrored = kFalse; // Did the DI subsystem have an error?
   nInTimer::tInTimer_Error_t scanOverrun = nInTimer::kNO_ERROR;
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

   if (deviceInfo->isSimultaneous) nNISTC3::initializeSimultaneousXSeries(device, status);

   // Determine the size of Port 0
   port0Length = deviceInfo->port0Length;

   if (port0Length == 32)
   {
      // Port 0 has 32 lines for hardware-timed DIO
      lineMaskPort0 &= static_cast<u32>(nDI::nDI_FIFO_Data_Register::nCDI_FIFO_Data::kMask);
      dataWidth = nDI::kDI_FourBytes;
      dataLane = nDI::kDI_DataLane0;  // Doesn't matter with 4-byte data, ignored
      sampleSizeInBytes = 4;
   }
   else
   {
      // Port 0 has 8 lines for hardware-timed DIO
      lineMaskPort0 &= static_cast<u32>(nDI::nDI_FIFO_Data_Register8::nCDI_FIFO_Data8::kMask);
      dataWidth = nDI::kDI_OneByte;
      dataLane = nDI::kDI_DataLane0;
      sampleSizeInBytes = 1;
   }

   // Set the buffer parameters
   readSizeInBytes = sampsPerChan * sampleSizeInBytes;
   dmaSizeInBytes = dmaBufferFactor * readSizeInBytes;
   rawData.assign(readSizeInBytes, 0);

   //
   // Create subsystem helpers
   //

   nNISTC3::diHelper diHelper(device.DI, device.DI.DI_Timer, status);
   nNISTC3::dioHelper dioHelper(device.DI, device.DO, status);
   dioHelper.setTristate(kTrue, status);
   nNISTC3::streamHelper streamHelper(device.DIStreamCircuit, device.CHInCh, status);

   //
   // Validate the Feature Parameters
   //

   // Nothing else to validate

   /*********************************************************************\
   |
   |   Program peripheral subsystems
   |
   \*********************************************************************/

   // No peripheral subsystems used in this example

   /*********************************************************************\
   |
   |   Program the DI subsystem
   |
   \*********************************************************************/

   //
   // Reset the DI subsystem
   //

   diHelper.reset(status);
   dioHelper.reset(NULL, 0, status);

   //
   // Program DI timing
   //

   // Enter timing configuration mode
   device.DI.DI_Timer.Reset_Register.writeConfiguration_Start(kTrue, &status);

   // Disable external gating of the sample clock
   diHelper.programExternalGate(
      nDI::kGate_Disabled,                  // No external pause signal
      nDI::kActive_High_Or_Rising_Edge,     // Don't care
      status);

   // Program the START1 signal (start trigger) to assert from a software rising edge
   diHelper.programStart1(
      nDI::kStart1_SW_Pulse,                // Set the line to software-driven
      nDI::kActive_High_Or_Rising_Edge,     // Make line active on rising...
      kTrue,                                //   ...edge (not high level)
      status);

   // Program the Convert signal (sample clock) to derive from the on-board clock
   diHelper.programConvert(
      nDI::kSampleClk_Internal,             // Drive the clock line from internal oscillator
      nDI::kActive_High_Or_Rising_Edge,     // Make the line active on rising edge
      status);

   // Program the sample and convert clock timing specifications
   timingConfig.setAcqLevelTimingMode(nNISTC3::kInTimerContinuous, status); // This is a continuous measurement
   timingConfig.setUseSICounter(kTrue, status);  // Use SI for internal sample clocking
   timingConfig.setSamplePeriod(samplePeriod, status);
   timingConfig.setSampleDelay(sampleDelay, status);
   timingConfig.setNumberOfSamples(0, status);   // Doesn't matter since this is continuous

   // Push the timing specification to the device
   diHelper.getInTimerHelper(status).programTiming(timingConfig, status);

   // Set the FIFO layout
   device.DI.DI_Mode_Register.setDI_DataWidth(dataWidth, &status);
   device.DI.DI_Mode_Register.setDI_Data_Lane(dataLane, &status);
   device.DI.DI_Mode_Register.flush(&status);

   // Clear the DI FIFO
   diHelper.getInTimerHelper(status).clearFIFO(status);

   //
   // Program DI channels
   //

   dioHelper.configureLines(lineMaskPort0, nNISTC3::kCorrInput, status);

   // Leave timing configuration mode
   device.DI.DI_Timer.Reset_Register.writeConfiguration_End(kTrue, &status);

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
   |   Start the digital measurement
   |
   \*********************************************************************/

   //
   // Arm the DI subsystem
   //

   diHelper.getInTimerHelper(status).armTiming(timingConfig, status);

   //
   // Start the DI subsystem
   //

   printf("Starting continuous %.2f-second hardware-timed digital measurement.\n", runTime);
   printf("Reading %u-sample chunks from the %u-sample DMA buffer.\n", sampsPerChan, dmaSizeInBytes/sampleSizeInBytes);
   diHelper.getInTimerHelper(status).strobeStart1(status);

   /*********************************************************************\
   |
   |   Read and print data
   |
   \*********************************************************************/

   nNISTC3::nDIODataHelper::printHeader(0);

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
         dma->read(readSizeInBytes, &rawData[0], &bytesAvailable, allowOverwrite, &dataOverwritten, status);
         if (status.isNotFatal())
         {
            nNISTC3::nDIODataHelper::printData(rawData, readSizeInBytes, sampleSizeInBytes);
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

      // 3. Check for DI subsystem errors
      device.DI.DI_Timer.Status_1_Register.refresh(&status);
      scanOverrun = device.DI.DI_Timer.Status_1_Register.getOverrun_St(&status);
      fifoOverflow = device.DI.DI_Timer.Status_1_Register.getOverflow_St(&status);

      if (scanOverrun || fifoOverflow)
      {
         diErrored = kTrue;
      }

      if (diErrored || dmaErrored)
      {
         break;
      }

      // 4. Update the run time for this measurement
      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
   }

   /*********************************************************************\
   |
   |   Stop the digital measurement
   |
   \*********************************************************************/

   if (!diErrored)
   {
      // Stop and disarm the DI timing engine
      device.DI.DI_Timer.Command_Register.writeEnd_On_End_Of_Scan(kTrue, &status);

      // Wait for the last scan to complete
      rlpElapsedTime = 0;
      rlpStart = clock();
      while (device.DI.DI_Timer.Status_1_Register.readSC_Armed_St(&status))
      {
         // Spin on the SC Armed bit
         if (rlpElapsedTime > rlpTimeout)
         {
            printf("\n");
            printf("Error: DI timing engine did not stop within timeout.\n");
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
      dma->read(readSizeInBytes, &rawData[0], &bytesAvailable, allowOverwrite, &dataOverwritten, status);
      if (status.isNotFatal())
      {
         nNISTC3::nDIODataHelper::printData(rawData, readSizeInBytes, sampleSizeInBytes);
         bytesRead += readSizeInBytes;
      }
      else
      {
         dmaErr = status;
         dmaErrored = kTrue;
         break;
      }
   }

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
   if (scanOverrun)
   {
      printf("Error: DI sample clock overrun.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (fifoOverflow)
   {
      printf("Error: DI FIFO overflow.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (! (diErrored || dmaErrored))
   {
      printf("Finished continuous %.2f-second hardware-timed digital measurement.\n", runTime);
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
