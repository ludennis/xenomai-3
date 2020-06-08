/*
 * pllHelper.cpp
 *
 * Programs the PLL for X Series devices.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include "osiBus.h"
#include "pllHelper.h"

#include <stdio.h>
#include <time.h>

namespace nNISTC3
{
   pllHelper::pllHelper( tXSeries&        xseries,
                         tBoolean         isPCIe,
                         nMDBG::tStatus2& status ) :
      _xseries(xseries),
      _isPCIe(isPCIe),
      _pllIsEnabled(kFalse)
   {
      // 1. Setup a guaranteed backup PLL source.
      if (_isPCIe)
      {
         // For PCIe export the Reference Clock to a pin...
         // ...that can be used as a source for the PLL
         _xseries.Triggers.STAR_Trig_Register.writeStar_Trig_Output_Select(nTriggers::kStar_IntTriggerA7, &status);
         _xseries.Triggers.STAR_Trig_Register.writeStar_Trig_Pin_Dir(1, &status);
         _xseries.Triggers.IntTriggerA_OutputSelectRegister_i[7].writeIntTriggerA_i_Output_Select(nTriggers::kIntTriggerA_ReferenceClock, &status);

         _backupPLL_Source = nTriggers::kRefClkSrc_Star_Trigger;
      }
      else
      {
         // For PXIe, the PXIe_Clk100 is always available
         _backupPLL_Source = nTriggers::kRefClkSrc_PXIe_Clk100;
      }

      // 2. Set the lock count to give the PLL the maximum amount...
      // ...of time to synchronize with the external source
      _xseries.Triggers.PLL_LockCount_Register.writeRegister(0xFFFF, &status);

      // Set the backup PLL source
      _xseries.Triggers.Clock_And_Fout2_Register.setTB3_Select(nTriggers::kTB3_From_OSC, &status);
      _xseries.Triggers.Clock_And_Fout2_Register.setPLL_In_Source_Select(_backupPLL_Source, &status);
      _xseries.Triggers.Clock_And_Fout2_Register.flush(&status);

      // Flush the bus by reading a register, which gives the TB3 mux adequate
      // time to fully switch to the new source before the PLL is reset.
      _xseries.Triggers.PLL_Status_Register.readRegister(&status);

      // Clear the PLL Control Register
      _xseries.Triggers.PLL_Control_Register.writeRegister(0, &status);

      return;
   }

   pllHelper::~pllHelper()
   {
      if (_pllIsEnabled)
      {
         nMDBG::tStatus2 status;
         disablePLL(status);
      }
   }

   void pllHelper::enablePLL( const                                   PLL_Parameters_t& pllParameters,
                              nTriggers::tTrig_PLL_In_Source_Select_t desiredPLL_Source,
                              nMDBG::tStatus2&                        status )
   {
      if (status.isFatal()) return;

      // Wait variables
      f64 runTime = .01;
      f64 elapsedTime;
      clock_t start;

      // 1. Step 1 (set backup PLL source) was performed in the constructor.

      // 2. Step 2 (set the lock count ) was performed in the constructor.

      // 3. Select the source for the internal clock TB3 and select the Reference Clock In source for the PLL
      _xseries.Triggers.Clock_And_Fout2_Register.setTB3_Select(nTriggers::kTB3_From_PLL, &status);
      _xseries.Triggers.Clock_And_Fout2_Register.setPLL_In_Source_Select(desiredPLL_Source, &status);
      _xseries.Triggers.Clock_And_Fout2_Register.flush(&status);

      // 4. Select the range for the external reference signal
      // Set the pll multiplier and divisor fields
      _xseries.Triggers.PLL_Control_Register.setPLL_Filter_Range(nTriggers::kPLL_100MHz, &status);
      _xseries.Triggers.PLL_Control_Register.setPLL_RefDivisor(pllParameters.pllDivisor - 1, &status);
      _xseries.Triggers.PLL_Control_Register.setPLL_Multiplier(pllParameters.pllMultiplier - 1, &status);
      _xseries.Triggers.PLL_Control_Register.setPLL_OutputDivider(pllParameters.pllOutputDivider - 1, &status);
      _xseries.Triggers.PLL_Control_Register.flush(&status);

      // 5. Enable the PLL in a separate register access
      _xseries.Triggers.PLL_Control_Register.writePLL_Enable(kTrue, &status);
      _pllIsEnabled = kTrue;

      // 6a. Check if PLL Lock Timer has expired
      while (_xseries.Triggers.PLL_Status_Register.readPLL_TimerExpired(&status) == nTriggers::kPLL_NotLocked)
      {
         // Perform a short wait between polling the PLL_Status_Register
         start = clock();
         do
         {
            elapsedTime = static_cast<double>(clock() - start) / CLOCKS_PER_SEC;
         }
         while (elapsedTime < runTime);
      }

      // 6b. After the PLL Lock Timer expires, check if the PLL is locked
      if (_xseries.Triggers.PLL_Status_Register.readHW_Pll_Locked(&status) == nTriggers::kPLL_NotLocked)
      {
         disablePLL(status);

         printf("Error: PLL did not lock to the external reference clock.\n");
         status.setCode(kStatusBadParameter);
         return;
      }

      // 7. Enable PLL Interrupt. Even though interrupts are not enabled on the
      // host, this interrupt has the side effect of automatically switching
      // the TB3 mux back to the oscillator should the PLL fall out of lock.
      u32 value = (1 << nBrdServices::nGen_Interrupt1_Register::nPLL_OutOfLockIRQ_Enable::kOffset) |
                  (1 << nBrdServices::nGen_Interrupt1_Register::nPLL_OutOfLockIRQ_Ack::kOffset);
      _xseries.BusInterface.GlobalInterruptEnable_Register.writeGen_Interrupt_Enable(kTrue, &status);
      _xseries.BrdServices.Gen_Interrupt1_Register.writeRegister(value, &status);

      // 8. Read the HW_Pll_Locked bit again since it's possible that the PLL
      // fell out of lock while enabling the interrupt.
      if (_xseries.Triggers.PLL_Status_Register.readHW_Pll_Locked(&status) == nTriggers::kPLL_NotLocked)
      {
         disablePLL(status);

         printf("Error: PLL fell out of lock while enabling PLL interrupt.\n");
         status.setCode(kStatusBadParameter);
         return;
      }

      return;
   }

   void pllHelper::disablePLL(nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;
      if (!_pllIsEnabled) return;

      // Determine if the PLL fell out of lock
      if (_xseries.BusInterface.Gen_Interrupt_Status_Register.readPLL_OutOfLockEventSt(&status))
      {
         printf("Warning: PLL fell out of phase lock. TB3 switched back to on-board oscillator.\n");
      }

      // Select the source for the internal clock TB3
      _xseries.Triggers.Clock_And_Fout2_Register.setTB3_Select(nTriggers::kTB3_From_OSC, &status);
      _xseries.Triggers.Clock_And_Fout2_Register.setPLL_In_Source_Select(_backupPLL_Source, &status);
      _xseries.Triggers.Clock_And_Fout2_Register.flush(&status);

      // Acknowledge and disable the PLL interrupt
      u32 value = (1 << nBrdServices::nGen_Interrupt2_Register::nPLL_OutOfLockIRQ_Disable::kOffset) |
                  (1 << nBrdServices::nGen_Interrupt2_Register::nPLL_OutOfLockIRQ_Ack2::kOffset);
      _xseries.BrdServices.Gen_Interrupt2_Register.writeRegister(value, &status);
      _xseries.BusInterface.GlobalInterruptEnable_Register.writeGen_Interrupt_Disable(kTrue, &status);

      // Flush the bus by reading a register, which gives the TB3 mux adequate
      // time to fully switch to the new source before the PLL is disabled.
      _xseries.Triggers.PLL_Status_Register.readRegister(&status);

      // Disable the PLL
      _xseries.Triggers.PLL_Control_Register.writePLL_Enable(kFalse, &status);
      _pllIsEnabled = kFalse;

      return;
   }
} // nNISTC3
