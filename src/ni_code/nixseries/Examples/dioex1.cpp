/*
 * dioex1.cpp
 *   Single-point on-demand digital input and output on ports 0, 1, and 2
 *
 * dioex1 performs a software-timed digital loop-back operation and transfers
 * data via direct register accesses. After configuring the DI subsystem's
 * channel parameters, dioex1 updates the digital values of ports 0, 1 and 2
 * and then reads the updates back. Finally, dioex1 restores the hardware's
 * previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : digital IO
 * Channel
 * ! Channels        : port0, port1, port2
 *   Scaling         : none
 * ! Line mask       : all lines on each port
 * Timing
 * ! Sample mode     : single-point
 * ! Timing mode     : software-timed
 *   Clock source    : none
 * Trigger
 *   Start trigger   : none
 *   Reference trig  : none
 *   Pause trigger   : none
 * Read / Write Buffer
 * ! Data transfer   : programmed IO to/from registers
 *
 * External Connections
 *   None
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include <stdio.h>

// OS Interface
#include "osiBus.h"

// Chip Objects
#include "tXSeries.h"

// Chip Object Helpers
#include "devices.h"
#include "dio/dioHelper.h"
#include "dio/pfiDioHelper.h"
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
   u32 lineMaskPort0 = 0xFFFFFFFF; // Use all the lines on port0
   const u8 lineMaskPort1 = 0xFF; // Use all the lines on port1
   const u8 lineMaskPort2 = 0xFF; // Use all the lines on port2
   tBoolean tristateOnExit = kTrue;

   // Buffer parameters
   const u32 outputDataPort0 = 0x5A5A5A5A; // Value to write to port0
   const u32 outputDataPort1 = 0x99; // Value to write to port1
   const u32 outputDataPort2 = 0x66; // Value to write to port2

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Channel parameters
   u32 port0Length = 0;
   const u32 port1Length = 8;
   const u32 lineMaskPFI = (lineMaskPort2 << port1Length) | lineMaskPort1;

   // Buffer parameters
   const u32 outputDataPFI = (outputDataPort2 << port1Length) | outputDataPort1;
   u32 inputDataPort0 = 0x0; // Value of lines on port0
   u32 inputDataPFI = 0x0; // Value of lines on port1:2 (PFI0..15)

   // Bookkeeping
   nMDBG::tStatus2 status;
   tAddressSpace bar0;
   bar0 = bus->createAddressSpace(kPCI_BAR0);

   /*********************************************************************\
   |
   |   Initialize the operation
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
   }
   else
   {
      // Port 0 has 8 lines for hardware-timed DIO
      lineMaskPort0 &= static_cast<u32>(nDI::nDI_FIFO_Data_Register8::nCDI_FIFO_Data8::kMask);
   }

   //
   // Create subsystem helpers
   //

   nNISTC3::dioHelper dioHelper(device.DI, device.DO, status);
   dioHelper.setTristate(tristateOnExit, status);
   nNISTC3::pfiDioHelper pfiDioHelper(device.Triggers, status);
   pfiDioHelper.setTristate(tristateOnExit, status);

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
   |   Program the digital subsystem
   |
   \*********************************************************************/

   //
   // Reset the digital subsystem (port 0)
   //

   dioHelper.reset(NULL, 0, status);

   //
   // Program lines for output
   //

   dioHelper.configureLines(lineMaskPort0, nNISTC3::kOutput, status);

   /*********************************************************************\
   |
   |   Program the PFI subsystem
   |
   \*********************************************************************/

   //
   // Reset the PFI subsystem (ports 1 and 2)
   //

   pfiDioHelper.reset(NULL, 0, status);

   //
   // Program lines for output
   //

   pfiDioHelper.configureLines(lineMaskPFI, nNISTC3::kOutput, status);

   /*********************************************************************\
   |
   |   Write data
   |
   \*********************************************************************/

   printf("Writing 0x%0X to port0.\n", outputDataPort0 & lineMaskPort0);
   dioHelper.writePresentValue(lineMaskPort0, outputDataPort0, status);

   printf("Writing 0x%0X to port1.\n", outputDataPort1 & lineMaskPort1);
   printf("Writing 0x%0X to port2.\n", outputDataPort2 & lineMaskPort2);
   pfiDioHelper.writePresentValue(lineMaskPFI, outputDataPFI, status);

   /*********************************************************************\
   |
   |   Read and print data
   |
   \*********************************************************************/

   dioHelper.readPresentValue(lineMaskPort0, inputDataPort0, status);
   pfiDioHelper.readPresentValue(lineMaskPFI, inputDataPFI, status);

   printf("Read 0x%0X from port0.\n", inputDataPort0);
   printf("Read 0x%0X from port1.\n", inputDataPFI & lineMaskPort1);
   printf("Read 0x%0X from port2.\n", (inputDataPFI >> port1Length) & lineMaskPort2);

   /*********************************************************************\
   |
   |   Check for digital subsystem errors
   |
   \*********************************************************************/

   // Since the timing engine is not used, errors cannot be generated.
   // The PFI subsystem cannot generate errors either.

   /*********************************************************************\
   |
   |   Finalize the operation
   |
   \*********************************************************************/

   //
   // Restore the state of the device
   //

   // Nothing else to do: the helpers' destructors safely unwind device state.
}
