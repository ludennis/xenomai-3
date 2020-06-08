/*
 * tCHInChDMAChannel.cpp
 *
 * Provide a user-level interface for DMA Channel control
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include "dmaErrors.h"
#include "tCHInChDMAChannel.h"

// DMA buffer controllers
// Linear DMA buffer
#include "tLinearDMABuffer.h"
// Scatter-Gather DMA Buffer
#include "tScatterGatherDMABuffer.h"

namespace nNISTC3
{
   tCHInChDMAChannel::tCHInChDMAChannel( tXSeries&         device,
                                         tDMAChannelNumber channelNumber,
                                         nMDBG::tStatus2&  status ) :
      // Initialize data members
      _readIdx(0),
      _writeIdx(0),
      _lastwriteIdx(0),
      _direction(kIn),
      _topo(kNone),
      _size(0),
      _state(kUnknown),
      _buffer(NULL),
      // Initialize the DMA Channel on the CHInCh
      _controller(device, channelNumber, status)
   {
      if (status.isFatal()) return;

      // Put the channel in kIdle state
      reset(status);
   }

   tCHInChDMAChannel::~tCHInChDMAChannel()
   {
      // Stop the channel and free the buffer if it is running
      if (_state != kIdle)
      {
         nMDBG::tStatus2 status;
         reset(status);
      }
   }

   void tCHInChDMAChannel::start(nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      // If the channel is stopped, then the user is requesting to re-start it,
      // which implies it has already been configured properly and used. It
      // is safe to re-configure with the cached settings.
      if (_state == kStopped)
      {
         _config(status);
         if (status.isFatal()) return;
      }

      // If the channel hasn't been configured yet, don't start the channel.
      if (_state != kConfigured)
      {
         status.setCode(kWrongChannelState);
         return;
      }

      // Tell the CHInCh to start the DMA channel
      _controller.start(status);
      if (status.isFatal()) return;

      _state = kStarted;
   }

   void tCHInChDMAChannel::stop(nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      // If the channel is started, then stop it.
      if (_state == kStarted)
      {
         // Tell the CHInCh to stop the DMA channel
         _controller.requestStop(status);
      }

      _state = kStopped;
   }

   void tCHInChDMAChannel::reset(nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      // Stop the DMA channel
      stop(status);

      // Deallocate the DMA buffer if it is in active memory
      if (_buffer != NULL)
      {
         _buffer->free();
         delete _buffer;
         _buffer = NULL;
      }

      // Reset the bookkeeping data members
      _readIdx  = 0;
      _writeIdx = 0;
      _lastwriteIdx = 0;

      _state = kIdle;
   }

   u32 tCHInChDMAChannel::getControllerStatus(nMDBG::tStatus2& status) const
   {
      if (status.isFatal()) return 0;
      return _controller.getStatus(status);
   }

   void tCHInChDMAChannel::configure( iBus* const      bus,
                                      tDMATopology     topo,
                                      tDMADirection    direction,
                                      u32              size,
                                      nMDBG::tStatus2& status )
   {
      if (status.isFatal()) return;

      // Only allow idle channels to be configured
      if (_state != kIdle)
      {
         status.setCode(kWrongChannelState);
         return;
      }

      // The DMA buffer must be greater than zero
      if (size == 0)
      {
         status.setCode(kBadBuffer);
         return;
      }

      // Store DMA Channel configuration
      _direction = direction;
      _topo      = topo;
      _size      = size;

      // Allocate DMA buffer controller
      if (_topo == kNormal)
      {
         _buffer = new tLinearDMABuffer(bus);
      }
      else if ( _topo == kLinkChain     ||
                _topo == kLinkChainRing ||
                _topo == kReuseLinkRing    )
      {
         _buffer = new tScatterGatherDMABuffer(bus);
      }
      else
      {
         // The CHInCh does not support kRing mode
         // kNone mode is invalid
         status.setCode(kInvalidInput);
         return;
      }
      if (_buffer == NULL)
      {
         status.setCode(kBufferBadMemoryAllocation);
         return;
      }

      // Allocate DMA buffer memory
      _buffer->allocate(_size, status);
      if (status.isFatal())
      {
         delete _buffer;
         _buffer = NULL;
         return;
      }

      // Initialize the DMA buffer
      _buffer->initialize(_topo, _controller.getMaxLinkSize(), status);
      if (status.isFatal())
      {
         delete _buffer;
         _buffer = NULL;
         return;
      }

      // Program the CHInCh with the DMA Channel settings
      _config(status);
   }

   void tCHInChDMAChannel::read( u32              requestedBytes,
                                 u8*              userBuffer,
                                 u32*             bytesLeft,
                                 tBoolean         allowOverwrite,
                                 tBoolean*        dataOverwritten,
                                 nMDBG::tStatus2& status )
   {
      if (status.isFatal()) return;

      if ( !(_state == kStarted || _state == kStopped) )
      {
         status.setCode(kWrongChannelState);
         return;
      }

      // 1. Get number of bytes available in the buffer and validate
      u32 bytesInBuffer = _getBytesInBuffer(status);

      // 1a. If bytes transfered exceed the buffer size, the DMA buffer overflowed
      if (bytesInBuffer > _size && !allowOverwrite)
      {
         status.setCode(kBufferOverflow);
         return;
      }

      // 1b. If the user didn't pass a buffer, just return the bytes available
      if (requestedBytes == 0 || userBuffer == NULL)
      {
         *bytesLeft = bytesInBuffer;
         return;
      }

      // 1c. Check if there's enough data to satisfy user request
      if (requestedBytes > bytesInBuffer)
      {
         status.setCode(kDataNotAvailable);
         return;
      }

      // 1d. Check if the user request is too large
      if (requestedBytes > _size)
      {
         status.setCode(kInvalidInput);
         return;
      }

      // 2. Move Data from DMA Buffer
      if (allowOverwrite && bytesInBuffer > _size)
      {
         // Data has been overwritten, so set the buffer and index locations to
         // the most recent data
         _buffer->setLocation( (_writeIdx - requestedBytes) % _size );
         _readIdx = _writeIdx - requestedBytes;
         *dataOverwritten = kTrue;
      }
      _buffer->read(requestedBytes, userBuffer);

      // 3. Check for overflow again
      //    Note that the read index has not been updated yet. It's possible that
      //    there was an overflow while moving the data out of the DMA buffer.
      bytesInBuffer = _getBytesInBuffer(status);
      if (bytesInBuffer > _size && !allowOverwrite)
      {
         status.setCode(kBufferOverflow);
         return;
      }

      // 4. No overflow during data move. Update read index and recalculate the
      //    number of bytes available to return to the user.
      _readIdx += requestedBytes;
      bytesInBuffer = _getBytesInBuffer(status);
      *bytesLeft = bytesInBuffer;
   }

   void tCHInChDMAChannel::write( u32              requestedBytes,
                                  u8*              userBuffer,
                                  u32*             bytesLeft,
                                  tBoolean         allowRegeneration,
                                  tBoolean*        dataRegenerated,
                                  nMDBG::tStatus2& status )
   {
      if (status.isFatal()) return;

      if ( !(_state == kStarted || _state == kConfigured) )
      {
         status.setCode(kWrongChannelState);
         return;
      }

      // 1. Get number of bytes available/free in the buffer and validate
      u32 bytesInBuffer = _getBytesInBuffer(status);
      u32 bytesFree     = _size - bytesInBuffer;

      // 1a. If bytes transfered exceed the buffer size, the DMA buffer underflowed
      //     This occurs when the read index passes the write index
      if (bytesInBuffer > _size && !allowRegeneration)
      {
         status.setCode(kBufferUnderflow);
         return;
      }

      // 1b. If the user didn't pass a buffer, just return the bytes available
      if (requestedBytes == 0 || userBuffer == NULL)
      {
         *bytesLeft = bytesFree;
         return;
      }

      // 1c. Check if there's enough space to write user data
      if (requestedBytes > bytesFree)
      {
         status.setCode(kSpaceNotAvailable);
         return;
      }

      // 1d. Check if user request is too large
      if (requestedBytes > _size)
      {
         status.setCode(kInvalidInput);
         return;
      }

      // 2. Move Data to DMA Buffer
      if (allowRegeneration && bytesInBuffer > _size)
      {
         // Data has been regenerated, so set the buffer and index locations to
         // the most recent data
         _buffer->setLocation( (_readIdx - requestedBytes) % _size );
         _writeIdx = _readIdx - requestedBytes;
         *dataRegenerated = kTrue;
      }
      _buffer->write(requestedBytes, userBuffer);

      // 3. Check for underflow again
      //    Note that the write index has not been updated yet. It's possible that
      //    there was an underflow while moving the data into the DMA buffer.
      bytesInBuffer = _getBytesInBuffer(status);
      if (bytesInBuffer > _size && !allowRegeneration)
      {
         status.setCode(kBufferUnderflow);
         return;
      }

      // 4. No underflow during data move. Update write index and recalculate
      //    number of empty bytes to return to the user.
      _writeIdx += requestedBytes;
      bytesInBuffer = _getBytesInBuffer(status);
      bytesFree     = _size - bytesInBuffer;
      *bytesLeft = bytesFree;
   }

   void tCHInChDMAChannel::_config(nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      // Get the DMA buffer address
      u64 startAddress = _buffer->getStartAddress();
      u32 size = 0;
      switch(_topo)
      {
         case kNormal:
            size = _buffer->getSize();
            break;
         case kLinkChain:
         case kLinkChainRing:
         case kReuseLinkRing:
            size = _buffer->getLinkSize();
            break;
         default:
            status.setCode(kInvalidInput);
            return;
      }

      // Program the CHInCh with the DMA Channel address
      _controller.configure(_topo, startAddress, size, status);
      if (status.isFatal()) return;

      // Reset the bookkeeping data members
      _buffer->setLocation(0);
      _readIdx  = 0;
      _writeIdx = 0;

      _state = kConfigured;
   }

   u32 tCHInChDMAChannel::_getBytesInBuffer(nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return 0;

      if (_direction == kIn)
      {
         // Update _writeIdx value
         _writeIdx = _controller.getTransferCount(status);

         // Check if _writeIdx went down from last time. If so, is means that
         // we're headed for an erroneous kDataNotAvailable. Reset _writeIdx
         // to previous value if necessary. If not, update _lastwriteIdx
         if (_writeIdx < _lastwriteIdx)
         {
            _writeIdx = _lastwriteIdx;
         }
         else
         {
            _lastwriteIdx = _writeIdx;
         }
      }
      else
      {
         _readIdx = _controller.getTransferCount(status);
      }

      u32 bytesInBuffer = 0;

      if (_writeIdx < _readIdx)
      {
         bytesInBuffer = 0xFFFFFFFF - static_cast<u32>(_readIdx - _writeIdx);
         ++bytesInBuffer;
      }
      else
      {
         bytesInBuffer = static_cast<u32>(_writeIdx - _readIdx);
      }

      return bytesInBuffer;
   }
} // nNISTC3
