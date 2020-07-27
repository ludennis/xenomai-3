/*
 * pfiRtsiResetHelper.cpp
 *
 * Reset for PFI and RTSI lines results in all pins set to input.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include "pfiRtsiResetHelper.h"

// Chip Objects
#include "tTriggers.h"

namespace nNISTC3
{
   pfiRtsiResetHelper::pfiRtsiResetHelper( tTriggers&       triggers,
                                           tBoolean         preserveOutputState,
                                           nMDBG::tStatus2& status ) :
      _triggers(triggers),
      _preserveOutputState(preserveOutputState),
      _exampleStatus(status)
   {
   }

   pfiRtsiResetHelper::~pfiRtsiResetHelper()
   {
      nMDBG::tStatus2 status;

      reset(status);
   }

   void pfiRtsiResetHelper::reset(nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      if (!_preserveOutputState || _exampleStatus.isFatal())
      {
         _triggers.PFI_Direction_Register.writeRegister(0, &status);
         _triggers.RTSI_Trig_Direction_Register.writeRegister(0, &status);

         // Reset the digital filters
         _triggers.PFI_Filter_Register_0.writeRegister(0x0);
         _triggers.IntTriggerA_OutputSelectRegister_i[0].writeRegister(0x0, &status);
         _triggers.Trig_Filter_Settings2_Register.writeRegister(0x0);
         _triggers.Trig_Filter_Settings1_Register.writeRegister(0x0);
      }
   }
}
