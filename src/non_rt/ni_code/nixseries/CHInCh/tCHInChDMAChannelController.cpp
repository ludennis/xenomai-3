/*
 * tCHInChDMAChannelController.cpp
 *
 * Provide a chip-level interface for DMA programming on the CHInCh.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include "tCHInChDMAChannelController.h"

// Chip Objects
#include "tXSeries.h"

// DMA Error codes
#include "dmaErrors.h"

namespace nNISTC3
{
   tCHInChDMAChannelController::tCHInChDMAChannelController( tXSeries&         device,
                                                             tDMAChannelNumber channelNumber,
                                                             nMDBG::tStatus2&  status ) :
      _device(device),
      _controller(NULL)
   {
      if (status.isFatal()) return;

      // Make sure the chip is indeed the CHInCH
      const u32 signature = _device.CHInCh.CHInCh_Identification_Register.readRegister(&status);
      if (signature != nCHInCh::kCHInChSignature)
      {
         status.setCode(kBadCHInCh);
         return;
      }

      switch (channelNumber)
      {
         case kAI_DMAChannel:      _controller = &(_device.CHInCh.AI_DMAChannel);      break;
         case kCounter0DmaChannel: _controller = &(_device.CHInCh.Counter0DmaChannel); break;
         case kCounter1DmaChannel: _controller = &(_device.CHInCh.Counter1DmaChannel); break;
         case kCounter2DmaChannel: _controller = &(_device.CHInCh.Counter2DmaChannel); break;
         case kCounter3DmaChannel: _controller = &(_device.CHInCh.Counter3DmaChannel); break;
         case kDI_DMAChannel:      _controller = &(_device.CHInCh.DI_DMAChannel);      break;
         case kAO_DMAChannel:      _controller = &(_device.CHInCh.AO_DMAChannel);      break;
         case kDO_DMAChannel:      _controller = &(_device.CHInCh.DO_DMAChannel);      break;
         default:                  status.setCode(kBadDMAChannel);               return;
      }

      // Initialize this channel to a known state:
      //   Clear the interrupt enable bits, put the channel into Normal mode,
      //   and then issue a Stop.
      _controller->Channel_Control_Register.setRegister(0, &status);
      _controller->Channel_Control_Register.setMODE(nDMAController::kNormalDmaMode, &status);
      _controller->Channel_Control_Register.flush(&status);
      _controller->Channel_Operation_Register.setRegister(0, &status);
      _controller->Channel_Operation_Register.setSTOP(kTrue, &status);
      _controller->Channel_Operation_Register.flush(&status);

      // Update soft copies of the other registers
      _device.CHInCh.Host_Bus_Resource_Control_Register.refresh(&status);
   }

   tCHInChDMAChannelController::~tCHInChDMAChannelController()
   {
      // Nothing to destroy
   }

   void tCHInChDMAChannelController::configure( tDMATopology     topo,
                                                u64              memAddress,
                                                u32              memSize,
                                                nMDBG::tStatus2& status )
   {
      if (status.isFatal()) return;

      if (memAddress == NULL || memSize == 0)
      {
         status.setCode(kBadBuffer);
         return;
      }

      _topo = topo;

      u32 addressLow = static_cast<u32>(memAddress);
      #if k64BitKernel
         // Use 64-bit DMA addressing even though our buffer pointer is 32-bit
         _device.CHInCh.Host_Bus_Resource_Control_Register.setDMA_MA64(kTrue, &status);
         _device.CHInCh.Host_Bus_Resource_Control_Register.setDMA_LA64(kTrue, &status);
         u32 addressHigh = static_cast<u32>(memAddress >> 32);
      #else
         // Use 32-bit DMA addressing since our buffer pointer is 32-bit
         _device.CHInCh.Host_Bus_Resource_Control_Register.setDMA_MA64(kFalse, &status);
         _device.CHInCh.Host_Bus_Resource_Control_Register.setDMA_LA64(kFalse, &status);
      #endif // k64BitKernel

      // Allow the CHInCh to master the bus and set its addressing mode
      _device.CHInCh.Host_Bus_Resource_Control_Register.setIO_Master_Enable(kTrue, &status);
      _device.CHInCh.Host_Bus_Resource_Control_Register.flush(&status);

      // Set the DMA mode and tell the CHInCh the start address
      // of the host's DMA buffer
      nDMAController::tDMA_Mode_t dmaMode;
      if (_topo == kNormal)
      {
         // kNormal only needs the Memory Address Register programmed
         _controller->Channel_Memory_Address_Register_LSW.writeRegister(addressLow, &status);
         #if k64BitKernel
            _controller->Channel_Memory_Address_Register_MSW.writeRegister(addressHigh, &status);
         #endif
         dmaMode = nDMAController::kNormalDmaMode;
      }
      else
      {
         // kLinkChain, kLinkChainRing, kReuseLinkRing need Link Address and
         // Link Size registers programmed
         _controller->Channel_Link_Address_Register_LSW.writeRegister(addressLow, &status);
         #if k64BitKernel
            _controller->Channel_Link_Address_Register_MSW.writeRegister(addressHigh, &status);
         #endif
         _controller->Channel_Link_Size_Register.writeRegister(memSize, &status);
         dmaMode = nDMAController::kLinkChainDmaMode;
      }

      // Tell the CHInCh which DMA mode to use
      _controller->Channel_Control_Register.writeMODE(dmaMode, &status);

      // Clear the Total Transfer Count Status Register
      _controller->Channel_Operation_Register.writeCLR_TTC(kTrue, &status);
   }

   void tCHInChDMAChannelController::start(nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      // Start the transfer and clear the Total Transfer Count Status Register
      _controller->Channel_Operation_Register.setSTART(kTrue, &status);
      _controller->Channel_Operation_Register.setSTOP(kFalse, &status);
      _controller->Channel_Operation_Register.setCLR_TTC(kTrue, &status);
      _controller->Channel_Operation_Register.flush(&status);

      // Wait for the Link_Ready bit to assert for Link Chain topologies
      if (_topo == kLinkChain || _topo == kLinkChainRing || _topo == kReuseLinkRing)
      {
         do
         {
            // Spin on the Link_Ready bit
         } while ( !(_controller->Channel_Status_Register.readLink_Ready(&status)) );
      }
   }

   void tCHInChDMAChannelController::requestStop(nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      // Ask the CHInCh to stop
      _controller->Channel_Operation_Register.setSTART(kFalse, &status);
      _controller->Channel_Operation_Register.setSTOP(kTrue, &status);
      _controller->Channel_Operation_Register.setCLR_TTC(kFalse, &status);
      _controller->Channel_Operation_Register.flush(&status);

      // Wait for one of Error, Last_Link, or Done to indicate that the DMA
      // channel and host transfers have fully stopped.
      u32 chanStatus = 0;
      u32 done = 0;
      u32 last_link = 0;
      u32 error = 0;
      do
      {
         using namespace nDMAController::nChannel_Status_Register;
         chanStatus = getStatus(status);
         done = (chanStatus & nDone::kMask) >> nDone::kOffset;
         last_link = (chanStatus & nLast_Link::kMask) >> nLast_Link::kOffset;
         error = (chanStatus & nError::kMask) >> nError::kOffset;
      } while ( (done != 1) && (last_link != 1) && (error != 1) );
   }

   u32 tCHInChDMAChannelController::getStatus(nMDBG::tStatus2& status) const
   {
      if (status.isFatal()) return 0;
      return _controller->Channel_Status_Register.readRegister(&status);
   }

   u64 tCHInChDMAChannelController::getTransferCount(nMDBG::tStatus2& status) const
   {
      if (status.isFatal()) return 0;

      // Read the MSW, read the LSW, then read the MSW again. If the MSW
      // changed while reading the LSW, be conservative and assume that the
      // lsw is zero.
      //
      // Alternatively, we could use CHTTCLR (the latching version): i.e.
      // read CHTTCLR (LSW), read CHTTCLR (MSW), and compose the values
      // into 64-bit. The upper value is latched when the lower value is
      // read, so the data is guaranteed to be accurate. However, this method
      // is not thread-safe since the upper half could be re-latched if
      // another thread reads CHTTCLR (LSW).

      u64 msw1, lsw, msw2;

      msw1 = _controller->Channel_Total_Transfer_Count_Status_Register_MSW.readRegister(&status);
      lsw  = _controller->Channel_Total_Transfer_Count_Status_Register_LSW.readRegister(&status);
      msw2 = _controller->Channel_Total_Transfer_Count_Status_Register_MSW.readRegister(&status);

      if (msw1 != msw2)
      {
         lsw = 0;
      }

      return (msw2 << 32 | lsw);
   }

   u32 tCHInChDMAChannelController::getMaxLinkSize() const
   {
      return nCHInCh::kMaxLinkSize;
   }
} // nNISTC3
