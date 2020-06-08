/*
 * tDMABuffer.cpp
 *
 * Provide a generic interface for DMA buffer control.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include "tDMABuffer.h"

namespace nNISTC3
{
   tDMABuffer::tDMABuffer():
      _index (0)
   {
      // Nothing else to initialize
   };

   tDMABuffer::~tDMABuffer()
   {
      // Nothing to destroy
   }
} // nNISTC3
