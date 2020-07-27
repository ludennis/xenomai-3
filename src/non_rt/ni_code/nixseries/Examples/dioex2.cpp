/*
 * dioex2.cpp
 *   Finite hardware-timed digital input
 *
 * dioex2 reads a digital waveform using hardware timing and transfers data to
 * the host by reading directly from the FIFO. After configuring the DI
 * subsystem's timing and channel parameters, dioex2 sends a software start
 * trigger. Once the measurement is complete, dioex2 reads and prints the data.
 * Finally, dioex2 restores the hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : digital input
 * Channel
 *   Channels        : port0
 *   Scaling         : none
 *   Line mask       : all lines
 * Timing
 * ! Sample mode     : finite
 * ! Timing mode     : hardware-timed
 * ! Clock source    : on-board oscillator
 * ! Clock rate      : 20 kHz
 * Trigger
 * ! Start trigger   : software
 *   Reference trig  : none
 *   Pause trigger   : none
 * Read Buffer
 * ! Data transfer   : programmed IO from FIFO
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
   const u32 samplePeriod = 5000; // Rate: 100 MHz / 5000 = 20 kHz

   // Buffer parameters
   u32 sampsPerChan = 64;

   // Behavior parameters
   const f64 timeout = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Channel parameters
   u32 port0Length = 0;

   // Timing parameters
   nNISTC3::inTimerParams timingConfig;
   const u32 sampleDelay = 2; // Wait N TB3 ticks after the start trigger before clocking (N must be >= 2)

   // Buffer parameters
   u32 inputDiData = 0; // Holder for raw data from FIFO
   nNISTC3::nDIODataHelper::tFourByteInt_t value; // Raw data to u32 converter datatype

   u32 streamFifoSize = 0;
   u32 sampleSizeInBytes;
   u32 readSizeInBytes;
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

   // Ensure that the FIFO can hold a complete measurement
   streamFifoSize = streamHelper.getFifoSize();
   if (readSizeInBytes > streamFifoSize)
   {
      printf("Error: Number of samples (%u) is greater than the DI FIFO size (%u).\n", sampsPerChan, streamFifoSize/sampleSizeInBytes);
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
   timingConfig.setAcqLevelTimingMode(nNISTC3::kInTimerPostTrigger, status); // This is a finite measurement with samples after the trigger (eg post-trigger samples)
   timingConfig.setUseSICounter(kTrue, status);  // Use SI for internal sample clocking
   timingConfig.setSamplePeriod(samplePeriod, status);
   timingConfig.setSampleDelay(sampleDelay, status);
   timingConfig.setNumberOfSamples(sampsPerChan, status); // Take sampsPerChan points on each channel

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

   printf("Starting finite hardware-timed digital measurement.\n");
   diHelper.getInTimerHelper(status).strobeStart1(status);

   /*********************************************************************\
   |
   |   Wait for the measurement to complete
   |
   \*********************************************************************/

   start = clock();
   while (!device.DI.DI_Timer.Status_1_Register.readSC_TC_St(&status))
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

      // Spin on the Scan Counter Terminal Count Status
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
   // Read and print the data from the FIFO
   //

   for (u32 n=0; n<readSizeInBytes; n+=sampleSizeInBytes)
   {
      // Make sure there is data in the FIFO before reading from it
      if (!device.DI.DI_Timer.Status_1_Register.readFIFO_Empty_St(&status))
      {
         // Read from the FIFO
         if (sampleSizeInBytes == 1)
         {
            inputDiData = device.DI.DI_FIFO_Data_Register8.readRegister();
            rawData[n] = static_cast<u8>(inputDiData);
         }
         else
         {
            inputDiData = device.DI.DI_FIFO_Data_Register.readRegister();
            value.value = inputDiData;
            rawData[n+0] = value.byte[0];
            rawData[n+1] = value.byte[1];
            rawData[n+2] = value.byte[2];
            rawData[n+3] = value.byte[3];
         }
      }
   }

   nNISTC3::nDIODataHelper::printHeader(0);
   nNISTC3::nDIODataHelper::printData(rawData, readSizeInBytes, sampleSizeInBytes);

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
   if (!diErrored)
   {
      printf("Finished finite hardware-timed digital measurement.\n");
      printf("Read %u samples from the FIFO.\n", readSizeInBytes/sampleSizeInBytes);
   }

   //
   // Restore the state of the device
   //

   // Nothing else to do: the helpers' destructors safely unwind device state.
}
