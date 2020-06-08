/*
 * tScatterGatherDMABuffer.cpp
 *
 * Provide an interface for controlling a linked-list DMA buffer.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include "dmaErrors.h"
#include "tScatterGatherDMABuffer.h"

// For memcpy(), memset()
#include <string.h>

namespace nNISTC3
{
   tScatterGatherDMABuffer::tScatterGatherDMABuffer(iBus *bus):
      tDMABuffer(),
      _bus(bus),
      _memory(NULL),
      _size(0),
      _sgl()
   {
      // Nothing else to initialize
   }

   tScatterGatherDMABuffer::~tScatterGatherDMABuffer()
   {
      // Deallocate the SGL memory
      free();
   }

   void tScatterGatherDMABuffer::allocate(u32 size, nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      // Ask the OS Interface component to allocate the memory for SGL pages (nodes allocated by _sgl and lower)
      // NOTE: this is contiguous memory, but it will be partitioned into separate links.
      _memory = _bus->allocDMA(size);

      if (_memory == NULL)
      {
         status.setCode(kBufferBadMemoryAllocation);
         return;
      }

      _size = size;
      memset(_memory->getAddress(), 0x0, _size);
   }

   void tScatterGatherDMABuffer::free()
   {
      if (_memory != NULL)
      {
         // Ask the OS Interface component to deallocate the DMA buffer
         _bus->freeDMA(_memory);
         _memory = NULL;
      }
   }

   void tScatterGatherDMABuffer::initialize(tDMATopology topo, u32 maxLinkSize, nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      // Calculate the topology of the kernel DMA memory block to partition
      // it into equal-sized blocks
      u32 numNodes = 0;
      u32 pagesPerNode = 0;
      switch (topo)
      {
      case kLinkChain:
         numNodes = kNumLinkChainNodes;
         pagesPerNode = kNumPagesInLinkChainNode;
         break;
      case kLinkChainRing:
         numNodes = kNumLinkChainRingNodes;
         pagesPerNode = kNumPagesInLinkChainRingNode;
         break;
      case kReuseLinkRing:
         numNodes = kNumReuseLinkRingNodes;
         pagesPerNode = kNumPagesInReuseLinkRingNode;
         break;
      default:
         status.setCode(kInvalidInput);
         return;
      }
      u32 numPages = numNodes * pagesPerNode;
      u32 evenDmaDataSize = _size - (_size % numPages);
      u32 dmaPageSize = evenDmaDataSize / numPages;
      u32 dmaPageRem = _size - dmaPageSize * (numPages - 1);

      // Assemble a generalized SGL
      _buildGenericSGL(maxLinkSize, numPages, pagesPerNode, dmaPageSize, dmaPageRem, status);
      if (status.isFatal()) return;

      // Specialize for the list type
      switch (topo)
      {
      case kLinkChain:
         _sgl.setStopOnLastLink();
         break;
      case kLinkChainRing:
         _sgl.setWrapAround();
         break;
      case kReuseLinkRing:
         _sgl.setWrapAround();
         _sgl.setReuseLink();  // See function description in tCHInChSGL.h for valid usage
         break;
      }
   }

   // Since iBus->allocDMA() allocates contiguous memory, and _buildGenericSGL()
   // places the DMA links next to each other, the data is completely linearly
   // addressable as if it were a single coherant buffer. Rather than walk through
   // the node list, just read the data as if it were living in a linear buffer.
   void tScatterGatherDMABuffer::read(u32 requestedBytes, void *buffer)
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

   // Since iBus->allocDMA() allocates contiguous memory, and _buildGenericSGL()
   // places the DMA links next to each other, the buffer is completely linearly
   // addressable as if it were single and coherant. Rather than walk through
   // the node list, just write the data as if it were going to a linear buffer.
   void tScatterGatherDMABuffer::write(u32 requestedBytes, void *buffer)
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

   u64 tScatterGatherDMABuffer::getStartAddress() const
   {
      return _sgl.getCHLAR();
   }

   u32 tScatterGatherDMABuffer::getSize() const
   {
      return _size;
   }

   u32 tScatterGatherDMABuffer::getLinkSize() const
   {
      return _sgl.getCHLSR();
   }

   void tScatterGatherDMABuffer::_buildGenericSGL( u32              maxLinkSize,
                                                   u32              numPages,
                                                   u32              pagesPerNode,
                                                   u32              dmaPageSize,
                                                   u32              dmaPageRem,
                                                   nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      // Current index into the DMA memory block
      u64 dmaMemoryIndex = _memory->getPhysicalAddress();

      if (_sgl.isInitialized())
      {
         // Reset the SGL if it has been initiatlized before
         _sgl.reset(status);
      }
      else
      {
         // Allocate and create the first SGL node
         _sgl.initialize(_bus, maxLinkSize, status); // See function description in tCHInChSGL.h for valid usage
      }
      if (status.isFatal())
      {
         status.setCode(kBufferInitializationError);
         return;
      }

      // Build the middle of the list
      // Since iBus->allocDMA() allocates contiguous memory, just put DMA links next to each other
      for (u32 page = 0; page < numPages-1; ++page, dmaMemoryIndex += dmaPageSize)
      {
         if ( page && page % pagesPerNode == 0 )
         {
            // Create a new link and add a page to it
            _sgl.addTransfer(dmaMemoryIndex, dmaPageSize, status, kTrue);
            if (status.isFatal())
            {
               status.setCode(kBufferInitializationError);
               return;
            }
         }
         else
         {
            // Add a page to the current link
            _sgl.addTransfer(dmaMemoryIndex, dmaPageSize, status, kFalse);
            if (status.isFatal())
            {
               status.setCode(kBufferInitializationError);
               return;
            }
         }
      }

      // Add the last page
      _sgl.addTransfer(dmaMemoryIndex, dmaPageRem, status, kFalse);
      if (status.isFatal())
      {
         status.setCode(kBufferInitializationError);
         return;
      }
   }
} // nNISTC3
