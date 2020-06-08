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

#include <stdio.h>

// Data types
#include "osiTypes.h"

// iBus, acquireBoard(), releaseBoard()
#include "osiBus.h"

// The test function is implemented by each example
void test(iBus *bus);

i32 nOSINT100_kCCall main(i32 argc, char* argv[])
{
   if (argc <= 2)
   {
      printf("Invalid arguments.\n");
      printf("Usage: <executable> bus device\n");
      printf("  bus     PCI Bus Number\n");
      printf("  device  PCI Device Number\n");
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
   test(bus);

   releaseBoard(bus);
   return 0;
}
