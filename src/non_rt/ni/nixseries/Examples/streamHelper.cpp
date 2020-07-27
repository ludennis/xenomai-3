/*
 * streamHelper.cpp
 *
 * Programs the Stream Circuit for DMA transfers.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include <time.h>
#include "streamHelper.h"

namespace nNISTC3
{
   streamHelper::streamHelper( tStreamCircuitRegMap& streamCircuit,
                               tCHInCh&              CHInCh,
                               nMDBG::tStatus2&      status ) :
      _streamCircuit(streamCircuit),
      _CHInCh(CHInCh),
      _isEnabled(kFalse),
      _isConfigured(kFalse),
      _usingCISTCR(kFalse)
   {
      reset(status);
      if (status.isFatal()) return;

      // Determine the maximum payload supported by the CHInCh
      u32 maxPayloadExp = _CHInCh.IO_Port_Resource_Description_Register.readIOMPS(&status);
      u32 maxPayloadSize = (1 << maxPayloadExp);

      // Determine the maximum payload supported by the subsystem's Stream Circuit
      _streamFifoSize = _streamCircuit.StreamFifoSizeReg.readStreamFifoSize(&status);

      // Determine the maximum PCIe transaction limit supported by the subsystem's Stream Circuit
      _transactionLimit = _streamCircuit.StreamTransactionLimitReg.readMaxTransactionLimit(&status);

      // Determine the maximum payload supported by the X Series device
      _payloadSize = (maxPayloadSize > _streamFifoSize) ? (u16)_streamFifoSize : (u16)maxPayloadSize;
   }

   void streamHelper::configureForInput( tBoolean                   useCISTCR,
                                         nNISTC3::tDMAChannelNumber dmaChannel,
                                         nMDBG::tStatus2&           status )
   {
      if (status.isFatal()) return;

      _configure(kTrue, useCISTCR, dmaChannel, status);
   }

   void streamHelper::configureForOutput( tBoolean                   useCISTCR,
                                          nNISTC3::tDMAChannelNumber dmaChannel,
                                          nMDBG::tStatus2&           status )
   {
      if (status.isFatal()) return;

      _configure(kFalse, useCISTCR, dmaChannel, status);
   }

   void streamHelper::enable(nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      if (_isConfigured && !_isEnabled)
      {
         _streamCircuit.StreamControlStatusReg.writeDataTransferEnable(kTrue, &status);
         _isEnabled = kTrue;
      }
      else
      {
         status.setCode(kStatusWrongState);
         return;
      }
   }

   void streamHelper::modifyTransferSize(i32 sizeDelta, nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      if (_usingCISTCR && _isConfigured)
      {
         _streamCircuit.StreamAdditiveTransferCountReg.writeRegister(sizeDelta, &status);
      }
      else
      {
         status.setCode(kStatusWrongState);
         return;
      }
   }

   u32 streamHelper::getFifoSize()
   {
      return _streamFifoSize;
   }

   tBoolean streamHelper::fifoIsEmpty(nMDBG::tStatus2& status)
   {
      return static_cast<tBoolean>(_streamCircuit.StreamControlStatusReg.readFifoEmpty(&status));
   }

   void streamHelper::disable(nMDBG::tStatus2& status)
   {
      if (_isEnabled)
      {
         reset(status);
      }
   }

   void streamHelper::reset(nMDBG::tStatus2& status)
   {
      // Reset the Streaming Circuit
      u32 commandValue = 0;
      commandValue |= nStreamCircuitRegMap::nStreamControlStatusReg::nCISTCR_Clear::kMask;
      commandValue |= nStreamCircuitRegMap::nStreamControlStatusReg::nStreamCircuitReset::kMask;
      commandValue |= nStreamCircuitRegMap::nStreamControlStatusReg::nInvalidPacketClear::kMask;
      _streamCircuit.StreamControlStatusReg.writeRegister(commandValue, &status);

      // Wait for the reset to complete
      f64 elapsedTime = 0;
      f64 timeout = 5;
      clock_t start = clock();
      while (! _streamCircuit.StreamControlStatusReg.readStreamCircuitResetComplete(&status))
      {
         // Spin on the Stream Circuit Reset Complete bit
         if (elapsedTime > timeout)
         {
            status.setCode(kStatusRLPTimeout);
            return;
         }
         elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
      }

      _isEnabled = kFalse;
      _usingCISTCR = kFalse;
      _isConfigured = kFalse;
   }

   streamHelper::~streamHelper()
   {
      nMDBG::tStatus2 status;
      if (_isEnabled)
      {
         disable(status);
      }
      reset(status);
   }

   void streamHelper::_configure( tBoolean                   isInput,
                                  tBoolean                   useCISTCR,
                                  nNISTC3::tDMAChannelNumber dmaChannel,
                                  nMDBG::tStatus2&           status )
   {
      if (status.isFatal()) return;

      if (_isEnabled || _isConfigured)
      {
         reset(status);
      }
      if (status.isFatal()) return;

      // Set the minimum and maximum paylod sizes (in bytes) for best high-throughput transfers
      _streamCircuit.StreamTransferLimitReg.setStreamMinPayloadSize(_payloadSize, &status);
      _streamCircuit.StreamTransferLimitReg.setStreamMaxPayloadSize(_payloadSize, &status);
      _streamCircuit.StreamTransferLimitReg.flush(&status, kTrue);

      if (isInput)
      {
         // Reduce pre-emptive FIFO flushing to maximize input throughput
         _streamCircuit.StreamEvictionReg.setDisableEviction(kFalse, &status);
         _streamCircuit.StreamEvictionReg.setEvictionTime(nStreamCircuitRegMap::nStreamEvictionReg::nEvictionTime::kMask, &status);
         _streamCircuit.StreamEvictionReg.flush(&status);
      }
      else
      {
         // Set the maximum transaction limit to maximize output throughput
         _streamCircuit.StreamTransactionLimitReg.setTransactionLimit(_transactionLimit, &status);
         _streamCircuit.StreamTransactionLimitReg.flush(&status, kTrue);
      }

      if (useCISTCR)
      {
         _usingCISTCR = kTrue;

         // Enable the Stream Transfer Count Register
         _streamCircuit.StreamControlStatusReg.writeCISTCR_Enable(kTrue, &status);
      }
      else
      {
         _usingCISTCR = kFalse;

         // Disable the Stream Transfer Count Register
         _streamCircuit.StreamControlStatusReg.writeCISTCR_Disable(kTrue, &status);
      }

         // Set the DMA channel for the Stream Circuit
      _streamCircuit.DMAChannel[0].writeDMAChannelNumber(dmaChannel, &status);

      _isConfigured = kTrue;
   }

} // nNISTC3
