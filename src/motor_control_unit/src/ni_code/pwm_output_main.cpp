/*
 * main.cpp
 *
 * Demonstrate an example for an X Series device.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include <cstdlib>
#include <stdio.h>

// Data types
#include "osiTypes.h"

// iBus, acquireBoard(), releaseBoard()
#include "osiBus.h"

// The test function is implemented by each example
void test(iBus *bus, f32 dutyCycle, u32 frequency, f64 runTime);

int nOSINT100_kCCall main(int argc, char* argv[])
{
   if (argc <= 5)
   {
      printf("Usage: pwm_output_main [bus number] [device number] [duty cycle (%)]"
        " [frequency (Hz)] [run time (second)\n");
      return 1;
   }

   // Create the board location string
   tChar brdLocation[256];
   sprintf(brdLocation, "PXI%s::%s::INSTR", argv[1], argv[2]);

   iBus* bus = NULL;
   bus = acquireBoard(brdLocation);
   if(bus == NULL)
   {
      printf("Could not access PCI device. Exiting.\n");
      return 1;
   }

   // Call example
   test(bus, atof(argv[3]), atol(argv[4]), atof(argv[5]));

   releaseBoard(bus);
   return 0;
}
