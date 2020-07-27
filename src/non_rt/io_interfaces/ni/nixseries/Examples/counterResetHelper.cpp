/*
 * counterReset.cpp
 *
 * Helper for reseting counters.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include "counterResetHelper.h"

// Chip Objects
#include "tCounter.h"

namespace nNISTC3
{
   counterResetHelper::counterResetHelper( tCounter& counter,
                                           tBoolean preserveOutputState,
                                           nMDBG::tStatus2& status ) :
      _counter(counter),
      _preserveOutputState(preserveOutputState),
      _exampleStatus(status)
   {
   }

   counterResetHelper::~counterResetHelper()
   {
      nMDBG::tStatus2 status;

      reset(kFalse, status);
   }

   void counterResetHelper::reset( tBoolean         initialReset,
                                   nMDBG::tStatus2& status )
   {
      if (status.isFatal()) return;

      if (initialReset)
      {
         if (!_preserveOutputState)
         {
            _counter.Gi_Command_Register.writeGi_Reset(kTrue, &status);
         }
      }
      else
      {
         if (_preserveOutputState && _exampleStatus.isNotFatal())
         {
            _counter.Gi_Mode_Register.writeGi_Output_Mode(nCounter::kTC_mode, &status);
            _counter.Gi_Mode_Register.writeGi_Output_Mode(nCounter::kToggle_Output_On_TC, &status);
         }
         else
         {
            _counter.Gi_Command_Register.writeGi_Reset(kTrue, &status);
         }
      }

      _counter.Gi_Interrupt2_Register.writeRegister(0xFFFFFFFF, &status);
      _counter.Gi_DMA_Config_Register.writeGi_DMA_Reset(kTrue, &status);
   }
}
