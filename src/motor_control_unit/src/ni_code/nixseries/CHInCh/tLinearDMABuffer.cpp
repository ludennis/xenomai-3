/*
 * tLinearDMABuffer.cpp
 *
 * Provide an interface for controlling a contiguous DMA buffer.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include "dmaErrors.h"
#include "tLinearDMABuffer.h"

// For memcpy(), memset()
#include <string.h>

namespace nNISTC3
{
   tLinearDMABuffer::tLinearDMABuffer(iBus *bus):
      tDMABuffer(),
      _bus(bus),
      _memory(NULL),
      _size(0)
   {
      // Nothing else to initialize
   }

   tLinearDMABuffer::~tLinearDMABuffer()
   {
      // Deallocate the DMA buffer
      free();
   }

   void tLinearDMABuffer::allocate(u32 size,nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      // Ask the OS Interface component to allocate a DMA buffer
      _memory = _bus->allocDMA(size);

      if (_memory == NULL)
      {
         status.setCode(kBufferBadMemoryAllocation);
         return;
      }

      _size = size;
      memset(_memory->getAddress(), 0x0, _size);
   }

   void tLinearDMABuffer::free()
   {
      if (_memory != NULL)
      {
         // Ask the OS Interface component to deallocate the DMA buffer
         _bus->freeDMA(_memory);
         _memory = NULL;
      }
   }

   void tLinearDMABuffer::initialize(tDMATopology topo, u32 maxLinkSize, nMDBG::tStatus2& status)
   {
      // Nothing to intialize for contiguous DMA buffers
      nNIOSINT100_mUnused(topo);
      nNIOSINT100_mUnused(maxLinkSize);
      nNIOSINT100_mUnused(status);
   }

   void tLinearDMABuffer::read(u32 requestedBytes, void *buffer)
   {
      u64 current = getLocation();
      u64 end     = (current + requestedBytes) % _size;

      u8 *address = (u8 *)_memory->getAddress();

      size_t offset = 0;

      if (end <= current)
      {
         offset = (size_t)(_size - current);
         memcpy ( buffer, address + current , offset);

         buffer = (void*) ((u8 *) buffer + offset);
         current = 0;
      }

      memcpy(buffer, address + current , (size_t)(end - current));

      setLocation (end);
   }


   void tLinearDMABuffer::write(u32 requestedBytes, void *buffer)
   {
      u64 current = getLocation ();
      u64 end     = (current + requestedBytes) % _size;

      u8 *address = (u8 *) _memory->getAddress();

      size_t offset = 0;

      if (end <= current)
      {
         offset = (size_t)(_size - current);
         memcpy (address + current , buffer, offset);

         buffer = (void*) ((u8 *) buffer + offset);
         current = 0;
      }

      memcpy(address + current, buffer, (size_t)(end - current));

      setLocation(end);
   }

   u64 tLinearDMABuffer::getStartAddress() const
   {
      return _memory->getPhysicalAddress();
   }

   u32 tLinearDMABuffer::getSize() const
   {
      return _size;
   }

   u32 tLinearDMABuffer::getLinkSize() const
   {
      return 0;
   }
} // nNISTC3
