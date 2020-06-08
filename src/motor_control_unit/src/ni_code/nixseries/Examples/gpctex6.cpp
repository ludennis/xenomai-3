/*
 * gpctex6.cpp
 *   Pulse train output using the frequency generator
 *
 * gpctex6 generates a pulse train using implicit timing and transfers data to
 * the device by writing directly to the frequency generator registers. After
 * configuring the generator's timebase and divider parameters, gpctex6 enables
 * the output. For 10 seconds, the pulse train generates until gpctex6 disables
 * the generator. Finally, gpctex6 restores the hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : pulse train generation
 * Channel
 * ! Channels        : frequency generator output
 *   Scaling         : none
 * ! Output terminal : PFI0
 * Timing
 *   Sample mode     : single-point
 *   Timing mode     : implicit
 * ! Clock source    : on-board oscillator
 * Trigger
 *   Start trigger   : not supported
 *   Reference trig  : not supported
 *   Pause trigger   : not supported
 * Read Buffer
 * ! Data transfer   : programmed IO to registers
 * Behavior
 *   Runtime         : 10 seconds
 *
 * External Connections
 *   PFI0            : observe on an oscilloscope
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include <stdio.h>
#include <time.h>

// OS Interface
#include "osiBus.h"

// Chip Objects
#include "tXSeries.h"

// Chip Object Helpers
#include "devices.h"
#include "simultaneousInit.h"

void test(iBus* bus)
{
   enum tTimebase
   {
      kTB1,      // 20 MHz
      kTB1Div2,  // 10 MHz
      kTB2,      // 100 kHz
   };

   /*********************************************************************\
   |
   |   Example parameters
   |
   \*********************************************************************/

   //
   // Feature Parameters (modify these to exercise the example)
   //

   // Channel parameters
   const tTimebase timebase = kTB1;
   u8 divider = 4;

   // Behavior parameters
   const f64 runTime = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Channel parameters
   const u8 dividerMax = 16;
   const u8 dividerMin = 1;
   f64 timebaseRate = 1;
   switch (timebase)
   {
   case kTB1:     timebaseRate = 20000000; break;
   case kTB1Div2: timebaseRate = 10000000; break;
   case kTB2:     timebaseRate = 100000; break;
   }
   const f64 frequency = timebaseRate / divider;

   // Behavior parameters
   f64 elapsedTime = 0; // How long has the generation been running?
   clock_t start;

   // Bookkeeping
   nMDBG::tStatus2 status;
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

   // No subsystem helpers used in this example

   //
   // Validate the Feature Parameters
   //

   // Sanity check the divider
   if (divider < dividerMin || divider > dividerMax)
   {
      printf("Error: Invalid divider value (%u). Range: from %u to %u.\n", divider, dividerMin, dividerMax);
      status.setCode(kStatusBadSelector);
      return;
   }

   /*********************************************************************\
   |
   |   Program peripheral subsystems
   |
   \*********************************************************************/

   //
   // Configure and route PFI lines
   //

   device.Triggers.PFI_OutputSelectRegister_i[0].writePFI_i_Output_Select(nTriggers::kPFI_Freq_Out, &status);
   device.Triggers.PFI_Direction_Register.writePFI0_Pin_Dir(nTriggers::kPFI_Output, &status);

   /*********************************************************************\
   |
   |   Program the frequency generator
   |
   \*********************************************************************/

   //
   // Program the timebase and divider
   //

   switch (timebase)
   {
   case kTB1:
      device.Triggers.FOUT_Register.setFOUT_Timebase_Select(nTriggers::kFOUT_Src_IsFastTB, &status);
      device.Triggers.FOUT_Register.setFOUT_FastTB_DivideBy2(nTriggers::kFOUT_FastTB_isTB1, &status);
      break;

   case kTB1Div2:
      device.Triggers.FOUT_Register.setFOUT_Timebase_Select(nTriggers::kFOUT_Src_IsFastTB, &status);
      device.Triggers.FOUT_Register.setFOUT_FastTB_DivideBy2(nTriggers::kFOUT_FastTB_isTB1_DivBy2, &status);
      break;

   case kTB2:
      device.Triggers.FOUT_Register.setFOUT_Timebase_Select(nTriggers::kFOUT_Src_IsTB2, &status);
      // FOUT_FastTB_DivideBy2 doesn't matter
      break;
   }
   if (divider == 16)
   {
      // Zero means divide by 16
      divider = 0;
   }
   device.Triggers.FOUT_Register.setFOUT_Divider(divider, &status);
   device.Triggers.FOUT_Register.flush(&status);

   /*********************************************************************\
   |
   |   Start the frequency generation
   |
   \*********************************************************************/

   printf("Generating %.2f Hz pulse train for %.2f seconds.\n", frequency, runTime);
   device.Triggers.FOUT_Register.writeFOUT_Enable(nTriggers::kFOUT_Enabled, &status);

   /*********************************************************************\
   |
   |   Wait for the generation to complete
   |
   \*********************************************************************/

   start = clock();
   while (elapsedTime < runTime)
   {
      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
   }

   /*********************************************************************\
   |
   |   Check for frequency generator errors
   |
   \*********************************************************************/

   // The frequency generator cannot generate errors

   /*********************************************************************\
   |
   |   Stop the frequency generation
   |
   \*********************************************************************/

   device.Triggers.FOUT_Register.writeFOUT_Enable(nTriggers::kFOUT_Disabled, &status);

   /*********************************************************************\
   |
   |   Finalize the generation
   |
   \*********************************************************************/

   //
   // Print run-time summary
   //

   printf("Finished generating %.2f Hz pulse train.\n", frequency);

   //
   // Restore the state of the device
   //

   device.Triggers.PFI_Direction_Register.writePFI0_Pin_Dir(nTriggers::kPFI_Input, &status);
}
