/*
 * tCHInChSGLChunkyLink.cpp
 *
 * Define the CHInCh chunky link interface.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include "tCHInChSGLChunkyLink.h"

// For memset()
#include <string.h>

// tDMAError
#include "dmaErrors.h"

namespace nNISTC3
{
   tCHInChSGLChunkyLink::tCHInChSGLChunkyLink( iBus* const           bus,
                                               tCHInChSGLChunkyLink* previousLink,
                                               u32                   maxSize,
                                               nMDBG::tStatus2&      status ) :
      _bus(bus),
      _maxSize(maxSize),
      _linkMemory(NULL),
      _linkMemorySize(0),
      _bytesUsed(0),
      _previousLink(previousLink),
      _nextLink(NULL)
   {
      // NOTE: The CHInCh hardware implemenation requires that the chunky links
      // be aligned to an 8-byte boundary at minimum, however the CHInCh can
      // perform up to a 512-byte read in order to fetch chunky links, but only
      // if the chunky link is aligned on a 512-byte boundary. It is important
      // to get the chunky links aligned to get the maximum possible performance
      // during high speed streaming, otherwise if a chunky spans a
      // 512-alignment, the CHInCh will need two PCIe bus transactions to fetch
      // the whole link (rather than just one). Note: the implementation of
      // iBus::allocDMA() cannot guarantee such byte-alignment.

      _linkMemory = _bus->allocDMA(_maxSize);
      if (_linkMemory == NULL)
      {
         status.setCode(kBufferBadMemoryAllocation);
      }

      // Enforce 8-byte alignment for link memory addresses
      if (_linkMemory->getPhysicalAddress() % 8 != 0)
      {
         status.setCode(kBufferBadAlignment);
      }

      // Since we don't know if any other links will be added after us, we can't
      // fill in the header right now, but if there is a link before us, we
      // can tell it that we exist now.
      if (status.isNotFatal())
      {
         memset(_linkMemory->getAddress(), 0, _maxSize);
         updatePrevious();
      }

      _linkMemorySize = _maxSize;
      _bytesUsed = status.isFatal() ? 0 : sizeof(tHeaderData);
   }

   tCHInChSGLChunkyLink::~tCHInChSGLChunkyLink()
   {
      if (_linkMemory)
      {
         _bus->freeDMA(_linkMemory);
         _linkMemory = NULL;
      }
      return;
   }

   void tCHInChSGLChunkyLink::addDataTransferLink( u64              physicalAddress,
                                                   u32              size,
                                                   nMDBG::tStatus2& status )
   {
      if (status.isFatal()) return;

      // Fail if there isn't enough room for the transfer link.
      if (getNumberOfBytesAvailable() < sizeof(tTransferLinkData))
      {
         status.setCode(kDMAChunkyLinkMemoryFull);
         return;
      }

      // Add the transfer link.
      tTransferLinkData* transferLink = reinterpret_cast<tTransferLinkData*>(
         reinterpret_cast<u8*>(getVirtualAddress()) + _bytesUsed);

      transferLink->transferSize = size;
      transferLink->physAddr = physicalAddress;
      transferLink->linkIdent = tCHInChSGLChunkyLink::TransferLink;
      _bytesUsed += sizeof(tTransferLinkData);

      updatePrevious();
   }

   void tCHInChSGLChunkyLink::setNext(tCHInChSGLChunkyLink* next)
   {
      tHeaderData* header = getHeader();

      _nextLink = next;
      if (_nextLink)
      {
         _nextLink->_previousLink = this;
         _nextLink->updatePrevious();
      }
      else
      {
         memset(header, 0, sizeof(tHeaderData));
         header->isLastLink = tHeaderData::LastLink;
      }
   }

   void tCHInChSGLChunkyLink::setReuseLink()
   {
      tHeaderData* header = getHeader();
      header->reuseLink = tHeaderData::doReuseLink;
   }

   tBoolean tCHInChSGLChunkyLink::getReuseLink() const
   {
      tHeaderData* header = getHeader();
      return header->reuseLink == tHeaderData::doReuseLink;
   }

   void tCHInChSGLChunkyLink::updatePrevious()
   {
      // Tell the previous node that it isn't the end, to point to this one and
      // how big it is.
      if (_previousLink)
      {
         // Infinite loop if the next line is replaced with _previousLink->setNext(this);
         _previousLink->_nextLink = this;
         _previousLink->getHeader()->isLastLink = tHeaderData::NotLastLink;
         _previousLink->getHeader()->reuseLink  = tHeaderData::doNotReuseLink;
         _previousLink->getHeader()->nextLinkSize = _bytesUsed;
         _previousLink->getHeader()->nextLink = getPhysicalAddress();
      }
   }
} // nNISTC3
