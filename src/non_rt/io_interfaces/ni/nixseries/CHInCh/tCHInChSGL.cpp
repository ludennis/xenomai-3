/*
 * tCHInChSGL.cpp
 *
 * Define the CHInCh Scatter-Gather List interface.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include "dmaErrors.h"
#include "tCHInChSGL.h"
#include "tCHInChSGLChunkyLink.h"

namespace nNISTC3
{
   tCHInChSGL::tCHInChSGL() :
      _bus(NULL),
      _maxLinkSize(0),
      _initialized(kFalse),
      _head(NULL),
      _tail(NULL)
   {
      // Nothing else to initialize.
   }

   void tCHInChSGL::initialize( iBus* const      bus,
                                u32              maxLinkSize,
                                nMDBG::tStatus2& status )
   {
      if (status.isFatal()) return;

      _bus = bus,
      _maxLinkSize = maxLinkSize;

      // Set up the SGL.
      reset(status);

      if (status.isNotFatal())
      {
         _initialized = kTrue;
      }
   }

   void tCHInChSGL::reset(nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      // Clear out the SGL.
      clear();

      // We always need to have at least one chunky link in the list, so add it.
      _head = allocateChunkyLink(status);
   }

   tCHInChSGL::~tCHInChSGL()
   {
      // Clear out the SGL.
      clear();
   }

   tCHInChSGL::tLinkLocation tCHInChSGL::addTransfer( u64              physicalAddress,
                                                      u32              size,
                                                      nMDBG::tStatus2& status,
                                                      tBoolean         isNewLink )
   {
      if (status.isFatal())
      {
         return tLinkLocation();
      }

      if (size == 0)
      {
         status.setCode(kBadBuffer);
         return tLinkLocation();
      }

      // If creating a new link, and if the last one is not currently
      // empty, then make a new link. Otherwise, we use what we already have.
      if (isNewLink && !_tail->isEmpty())
      {
         allocateChunkyLink(status);
         if (status.isFatal())
         {
            return tLinkLocation();
         }
      }

      // Note that if the first AddTransfer operation failed, then we
      // assume it to mean that there wasn't enough space on the last
      // chunky link. Thus, we allocate a new one. Assuming the
      // allocation succeeded, then we will add the transfer link to
      // the new chunky link. We fully expect the second AddTransfer
      // operation to work, given that there *will* be enough space on
      // the chunky link for the new transfer link.
      nMDBG::tStatus2 testStatus;
      _tail->addDataTransferLink(physicalAddress, size, testStatus);

      if (testStatus.isFatal())
      {
         tCHInChSGLChunkyLink* newLink = allocateChunkyLink(status);
         if (status.isFatal())
         {
            return tLinkLocation();
         }
         newLink->addDataTransferLink(physicalAddress, size, status);
      }

      return tLinkLocation(_tail, 0);
   }

   void tCHInChSGL::setWrapAround()
   {
      _tail->setNext(_head);
   }

   void tCHInChSGL::setReuseLink()
   {
      this->setWrapAround();
      _tail->setReuseLink();
   }

   void tCHInChSGL::setStopOnLastLink()
   {
      _tail->setNext(NULL);
   }

   void tCHInChSGL::clear()
   {
      // Clean up the list.
      tCHInChSGLChunkyLink* link = _head;
      while (link)
      {
         tCHInChSGLChunkyLink* next = link->getNext();
         delete link;
         link = next == _head ? NULL : next;
      }
      _head = _tail = NULL;
   }

   u64 tCHInChSGL::getCHLAR() const
   {
      return _head->getPhysicalAddress();
   }

   u32 tCHInChSGL::getCHLSR() const
   {
      return _head->getSize();
   }

   tBoolean tCHInChSGL::isInitialized() const
   {
      return _initialized;
   }

   tCHInChSGL::operator tLinkInformation() const
   {
      return tLinkInformation(getCHLAR(), getCHLSR());
   }

   tCHInChSGLChunkyLink* tCHInChSGL::allocateChunkyLink(nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return NULL;

      // Make the new chunky link and add it to the end of the list.
      tCHInChSGLChunkyLink* newLink = new tCHInChSGLChunkyLink(_bus, _tail, _maxLinkSize, status);

      if (status.isFatal())
      {
         delete newLink;
         return NULL;
      }

      _tail = newLink;
      return newLink;
   }
} // nNISTC3
