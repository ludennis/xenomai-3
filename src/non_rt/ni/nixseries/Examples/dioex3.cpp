/*
 * dioex3.cpp
 *   Finite hardware-timed digital input with change detection
 *
 * dioex3 reads a digital waveform using the change detection circuit and
 * transfers data to the host using DMA. After configuring the DI subsystem's
 * timing and channel parameters, dioex3 configures and starts the DMA channel
 * before sending a software start trigger. Once the measurement is complete,
 * dioex3 shuts down DMA data and reads and prints the data. Finally, dioex3
 * restores the hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : digital input
 * Channel
 *   Channels        : port0
 *   Scaling         : none
 *   Line mask       : all lines
 * Timing
 *   Sample mode     : finite
 * ! Timing mode     : digital change detection
 * ! Clock source    : change detection circuit
 * ! Change mask     : rising  -- all lines
 * !                   falling -- all lines
 * Trigger
 *   Start trigger   : software
 *   Reference trig  : none
 *   Pause trigger   : none
 * Read Buffer
 * ! Data transfer   : scatter-gather DMA
 *   DMA buffer      : preserve unread samples
 * Behavior
 *   Timeout         : 10 seconds
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
   u32 risingEdgeMask = 0xFFFFFFFF;
   u32 fallingEdgeMask = 0xFFFFFFFF;

   // Buffer parameters
   u32 sampsPerChan = 1024;

   // Behavior parameters
   const f64 timeout = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Channel parameters
   u32 port0Length = 0;

   // Timing parameters
   nNISTC3::inTimerParams timingConfig;

   // Buffer parameters
   const tBoolean allowOverwrite = kFalse;
   tBoolean dataOverwritten = kFalse; // Has the DMA channel overwritten any unread data?
   u32 bytesRead = 0;      // Bytes read so far from the DMA buffer
   u32 bytesAvailable = 0; // Bytes in the DMA buffer
   const nNISTC3::tDMAChannelNumber dmaChannel = nNISTC3::kDI_DMAChannel;

   u32 sampleSizeInBytes;
   u32 dmaSizeInBytes;
   nDI::tDI_DataWidth_t dataWidth;
   nDI::tDI_Data_Lane_t dataLane;

   // Using a vector as a self deleting byte buffer
   std::vector<u8> rawData;

   // Behavior parameters
   f64 elapsedTime = 0; // How long has the measurement been running?
   clock_t start;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tBoolean diErrored = kFalse; // Did the DI subsystem have an error?
   nInTimer::tInTimer_Error_t scanOverrun = nInTimer::kNO_ERROR;
   nInTimer::tInTimer_Error_t fifoOverflow = nInTimer::kNO_ERROR;
   nInTimer::tInTimer_Error_t SC_TC_Error = nInTimer::kNO_ERROR;
   tBoolean dmaErrored = kFalse; // Did the DMA channel have an error?
   nMDBG::tStatus2 dmaErr;
   u32 dmaControllerStatus = 0;
   u32 dmaDone = 0; // Has the DMA channel processed the last DMA buffer link?
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
      risingEdgeMask &= static_cast<u32>(nDI::nDI_FIFO_Data_Register::nCDI_FIFO_Data::kMask);
      fallingEdgeMask &= static_cast<u32>(nDI::nDI_FIFO_Data_Register::nCDI_FIFO_Data::kMask);
      dataWidth = nDI::kDI_FourBytes;
      dataLane = nDI::kDI_DataLane0;  // Doesn't matter with 4-byte data, ignored
      sampleSizeInBytes = 4;
   }
   else
   {
      // Port 0 has 8 lines for hardware-timed DIO
      lineMaskPort0 &= static_cast<u32>(nDI::nDI_FIFO_Data_Register8::nCDI_FIFO_Data8::kMask);
      risingEdgeMask &= static_cast<u32>(nDI::nDI_FIFO_Data_Register8::nCDI_FIFO_Data8::kMask);
      fallingEdgeMask &= static_cast<u32>(nDI::nDI_FIFO_Data_Register8::nCDI_FIFO_Data8::kMask);
      dataWidth = nDI::kDI_OneByte;
      dataLane = nDI::kDI_DataLane0;
      sampleSizeInBytes = 1;
   }

   // Set the buffer parameters
   dmaSizeInBytes = sampsPerChan * sampleSizeInBytes;
   rawData.assign(dmaSizeInBytes, 0);

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

   // Ensure at least two scans
   if (sampsPerChan < 2)
   {
      printf("Error: Samples per channel (%u) must be 2 or greater.\n", sampsPerChan);
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

   // Set DIO Change Detection for DIO Port 0
   diHelper.programChangeDetection(
      risingEdgeMask,   // Port0 rising edge Change Detection mask
      fallingEdgeMask,  // Port0 falling edge Change Detection mask
      0,                // PFI rising edge Change Detection mask
      0,                // PFI falling edge Change Detection mask
      status);

   // Program the Convert signal (sample clock) to derive from the change detection circuit
   diHelper.programConvert(
      nDI::kSampleClk_DIO_ChgDetect,        // Drive the sample clock with DIO Change Detection Events
      nDI::kActive_High_Or_Rising_Edge,     // Make the line active on rising edge
      status);

   // Program the sample and convert clock timing specifications
   timingConfig.setAcqLevelTimingMode(nNISTC3::kInTimerPostTrigger, status); // This is a finite measurement with samples after the trigger (eg post-trigger samples)
   timingConfig.setUseSICounter(kFalse, status);               // Do not use SI for sample clocking since the dio change detection event is used
   timingConfig.setNumberOfSamples(sampsPerChan, status);      // Take sampsPerChan points

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

   printf("Starting finite change detection digital measurement.\n");
   diHelper.getInTimerHelper(status).strobeStart1(status);

   /*********************************************************************\
   |
   |   Wait for the measurement to complete
   |
   \*********************************************************************/

   // Wait for the DMA channel to finish processing the last DMA buffer link
   start = clock();
   while (!dmaDone)
   {
      // Check for DI subsystem errors
      device.DI.DI_Timer.Status_1_Register.refresh(&status);
      scanOverrun = device.DI.DI_Timer.Status_1_Register.getOverrun_St(&status);
      fifoOverflow = device.DI.DI_Timer.Status_1_Register.getOverflow_St(&status);
      SC_TC_Error = device.DI.DI_Timer.Status_1_Register.getSC_TC_Error_St(&status);

      if (scanOverrun || fifoOverflow || SC_TC_Error)
      {
         diErrored = kTrue;
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
   |   Stop the digital measurement
   |
   \*********************************************************************/

   // The DI subsystem has already been stopped and disarmed by hardware

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

   nNISTC3::nDIODataHelper::printHeader(0);

   // 1. Use the read() method to query the number of bytes in the DMA buffer
   dma->read(0, NULL, &bytesAvailable, allowOverwrite, &dataOverwritten, status);
   if (status.isFatal())
   {
      dmaErr = status;
      dmaErrored = kTrue;
   }

   // 2. If there is enough data in the buffer, read and print it
   else if (bytesAvailable >= dmaSizeInBytes)
   {
      dma->read(dmaSizeInBytes, &rawData[0], &bytesAvailable, allowOverwrite, &dataOverwritten, status);
      nNISTC3::nDIODataHelper::printData(rawData, dmaSizeInBytes, sampleSizeInBytes);

      if (status.isFatal())
      {
         dmaErr = status;
         dmaErrored = kTrue;
      }
      else
      {
         bytesRead += dmaSizeInBytes;
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
   if (SC_TC_Error)
   {
      printf("Error: SC reached terminal count twice without acknowledgement.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (! (diErrored || dmaErrored))
   {
      printf("Finished finite change detection digital measurement.\n");
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
