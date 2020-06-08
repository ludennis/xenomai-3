/*
 * eepromHelper.cpp
 *
 * Reads information from the EEPROM of the device and provides scaling methods.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include "eepromHelper.h"

#include <stdio.h>

// Chip Objects
#include "tXSeries.h"

namespace nNISTC3
{

   eepromHelper::eepromHelper( tXSeries&        xseries,
                               tBoolean         isSimultaneous,
                               u32              numberOfADCs,
                               u32              numberOfDACs,
                               nMDBG::tStatus2& status ) :
      _eeprom(xseries.getBusSpaceReference() + nCHInCh::kEEPROMOffset),
      _xseries(xseries),
      _isSimultaneous(isSimultaneous),
      _baseAddress(0),
      _numberOfADCs(numberOfADCs),
      _numberOfDACs(numberOfDACs),
      _serialNumber(0),
      _geographicalAddress(0),
      _stc3Revision(0),
      _extCalTime(0),
      _extCalTemp(0.0),
      _extCalVoltRef(0.0),
      _selfCalTime(0),
      _selfCalTemp(0.0),
      _selfCalVoltRef(0.0),
      _aiCalInfo(NULL),
      _aoCalInfo(NULL)
   {
      // This shouldn't happen!
      if (_numberOfADCs > 1 && !_isSimultaneous)
      {
         status.setCode(kStatusBadSelector);
         return;
      }

      u32 capabilitiesListFlagPtr = _readU32(kCapabilitiesListFlagPtrOffset);
      u32 capabilitiesListFlag = _readU32(capabilitiesListFlagPtr);
      u32 capabilitiesListOffset = 0;
      if (capabilitiesListFlag & 0x1)
      {
         capabilitiesListOffset = _readU32(kCapabilitiesListBPtrOffset);
      }
      else
      {
         capabilitiesListOffset = _readU32(kCapabilitiesListAPtrOffset);
      }

      _aiCalInfo = new tAICalibration[_numberOfADCs];
      _aoCalInfo = new tAOCalibration[_numberOfDACs];
      _traverseCapabilitiesList(capabilitiesListOffset, status);
   }

   eepromHelper::~eepromHelper()
   {
      delete[] _aoCalInfo;
      _aoCalInfo = NULL;

      delete[] _aiCalInfo;
      _aiCalInfo = NULL;
   }

   void eepromHelper::getAIScalingCoefficients(u32 ADCNumber, u32 intervalNumber, tAIScalingCoefficients& coefficients, nMDBG::tStatus2& status)
   {
      if (ADCNumber < _numberOfADCs)
      {
         if (_aiCalInfo[ADCNumber].intervals[intervalNumber].valid)
         {
            for (u32 i=0; i<kNumAIScalingCoefficients; ++i)
            {
               coefficients.c[i] = _aiCalInfo[ADCNumber].modes[0].coefficients[i] * _aiCalInfo[ADCNumber].intervals[intervalNumber].gain;
               if (i == 0)
               {
                  coefficients.c[i] += _aiCalInfo[ADCNumber].intervals[intervalNumber].offset;
               }
            }
         }
         else
         {
            status.setCode(kStatusBadInterval);
         }
      }
      else
      {
         status.setCode(kStatusBadSelector);
      }
   }

   void eepromHelper::getAOScalingCoefficients(u32 DACNumber, u32 intervalNumber, tAOScalingCoefficients& coefficients, nMDBG::tStatus2& status)
   {
      if (DACNumber < _numberOfDACs)
      {
         if (_aoCalInfo[DACNumber].intervals[intervalNumber].valid)
         {
            coefficients.c[0] = _aoCalInfo[DACNumber].intervals[intervalNumber].offset;
            coefficients.c[1] = _aoCalInfo[DACNumber].intervals[intervalNumber].gain;
         }
         else
         {
            status.setCode(kStatusBadInterval);
         }
      }
      else
      {
         status.setCode(kStatusBadSelector);
      }
   }

   void eepromHelper::_traverseCapabilitiesList(u32 nodeAddress, nMDBG::tStatus2& status)
   {
      u16 nextNodeAddress = 0;
      u16 currentNodeId = 0;

      while (nodeAddress != 0)
      {
         nextNodeAddress = _readU16(nodeAddress);
         currentNodeId = _readU16(nodeAddress + 2);

         switch (currentNodeId)
         {
            case kGeographicalAddressingNodeID:
               _parseGeographicalAddressingNode(nodeAddress, status);
               break;
            case kSerialNumberNodeID:
               _parseSerialNumberNode(nodeAddress, status);
               break;
            case kDeviceSpecificNodeID:
               _parseDeviceSpecificNode(nodeAddress, status);
               break;
            // Don't know what this is, just ignore it.
         }

         // Otherwise, keep going.  The last two bits of the next node ptr are
         // used to indicate whether the address read is absolute or relative.
         const u32 addressOptions = nextNodeAddress & 0x3;
         nextNodeAddress &= 0xFFFC;

         switch(addressOptions)
         {
            case 0:
               // Address is a 16-bit, absolute address.
               nodeAddress = nextNodeAddress;
               break;
            case 2:
               // Address is a 16-bit, relative address. The pointer is realive from the start of
               // the current node.
               nodeAddress += nextNodeAddress;
               break;
            default:
               // This should never happen (we do not support 32 bit addressing (relative = 1))
               status.setCode(kStatusBadSelector);
               return;
         }
      }
   }

   void eepromHelper::_parseGeographicalAddressingNode(u32 nodeAddress, nMDBG::tStatus2& status)
   {
      u32 gaPointer =  _readU32(nodeAddress + kGAPointerOffset);
      u32 gaInfo =  _readU32(nodeAddress + kGAInfoOffset);
      u32 gaShift = (gaInfo & kGAShiftMask) >> kGAShiftShift;
      u32 gaReadWidth = (gaInfo & kGAReadWidthMask) >> kGAReadWidthShift;
      u32 gaSpace = gaInfo & kGASpaceMask;
      if (gaSpace != kGALocalSpace)
      {
         status.setCode(kStatusBadSelector);
         return;
      }

      tBusSpaceReference bar0 = _xseries.getBusSpaceReference();
      switch (gaReadWidth)
      {
         case kGAReadWidth8Bit:
            _geographicalAddress = bar0.read8(gaPointer);
            break;
         case kGAReadWidth16Bit:
            _geographicalAddress = bar0.read16(gaPointer);
            break;
         case kGAReadWidth32Bit:
            _geographicalAddress = bar0.read8(gaPointer);
            break;
      }

      _geographicalAddress = (_geographicalAddress >> gaShift) & kGAMask;
   }

   void eepromHelper::_parseSerialNumberNode(u32 nodeAddress, nMDBG::tStatus2& status)
   {
      nNIOSINT100_mUnused(status);

      _serialNumber = _readU32(nodeAddress + kSerialNumberOffset);
   }

   void eepromHelper::_parseDeviceSpecificNode(u32 nodeAddress, nMDBG::tStatus2& status)
   {
      u32 dsnSize = _readU32(nodeAddress + kDSNSizeOffset);
      u32 dsnBodyFormat = _readU32(nodeAddress + kDSNBodyFormatOffset);
      // size includes the format and the trailing CRC
      u32 dsnBodySize = dsnSize - 2*sizeof(u32);

      u32 extCalAOffset = 0;
      u32 extCalBOffset = 0;
      u32 selfCalAOffset = 0;
      u32 selfCalBOffset = 0;

      for (u32 i=0; i<dsnBodySize;)
      {
         u32 value = 0;
         u32 id = 0;
         switch (dsnBodyFormat)
         {
            case kDSNBodyFormat16BitValueID:
               value = _readU16(nodeAddress + kDSNBodyOffset + i);
               id = _readU16(nodeAddress + kDSNBodyOffset + i + sizeof(u16));
               i += 2 * sizeof(u16);
               break;
            case kDSNBodyFormat32BitValueID:
               value = _readU32(nodeAddress + kDSNBodyOffset + i);
               id = _readU32(nodeAddress + kDSNBodyOffset + i + sizeof(u32));
               i += 2 * sizeof(u32);
               break;
            case kDSNBodyFormat16BitIDValue:
               id = _readU16(nodeAddress + kDSNBodyOffset + i);
               value = _readU16(nodeAddress + kDSNBodyOffset + i + sizeof(u16));
               i += 2 * sizeof(u16);
               break;
            case kDSNBodyFormat32BitIDValue:
               id = _readU32(nodeAddress + kDSNBodyOffset + i);
               value = _readU32(nodeAddress + kDSNBodyOffset + i + sizeof(u32));
               i += 2 * sizeof(u32);
               break;
            default:
               //This should never happen
               status.setCode(kStatusBadSelector);
               return;
         }

         switch (id)
         {
            case kExtCalAPtrID:
               extCalAOffset = value;
               break;
            case kExtCalBPtrID:
               extCalBOffset = value;
               break;
            case kSelfCalAPtrID:
               selfCalAOffset = value;
               break;
            case kSelfCalBPtrID:
               selfCalBOffset = value;
               break;
            case kASIC_RevisionID:
               _stc3Revision = value;
               break;
         }
      }

      // Get the Ext Cal information.
      u32 extCalAreaOffset = _getOffsetForPages(extCalAOffset, extCalBOffset, status);
      if (status.isFatal()) return;
      u32 extCalDataOffset = extCalAreaOffset + kCalDataOffset;
      _extCalTime = (static_cast<u64>(_readU32(extCalDataOffset + kCalTimeUpperOffset)) << 32) | _readU32(extCalDataOffset + kCalTimeLowerOffset) - kCalTimeBias;
      _extCalTemp = _readF32(extCalDataOffset + kCalTemperatureOffset);
      _extCalVoltRef = _readF32(extCalDataOffset + kCalVoltageRefOffset);

      // Now get the Self Cal information, these are the coefficients we actually use.
      u32 selfCalAreaOffset = _getOffsetForPages(selfCalAOffset, selfCalBOffset, status);
      if (status.isFatal()) return;
      u32 selfCalDataOffset = selfCalAreaOffset + kCalDataOffset;
      _selfCalTime = (static_cast<u64>(_readU32(selfCalDataOffset + kCalTimeUpperOffset)) << 32) | _readU32(selfCalDataOffset + kCalTimeLowerOffset) - kCalTimeBias;
      _selfCalTemp = _readF32(selfCalDataOffset + kCalTemperatureOffset);
      _selfCalVoltRef = _readF32(selfCalDataOffset + kCalVoltageRefOffset);

      u32 currentCalAddress = selfCalDataOffset + kCalCoefficientsOffset;
      for (u32 i=0; i<_numberOfADCs; ++i)
      {
         for (u32 j=0; j<kNumAIModes; ++j)
         {
            _readMode(currentCalAddress, _aiCalInfo[i].modes[j]);
            currentCalAddress += kModeSizeInBytes;
         }
         for (u32 j=0; j<kNumAIIntervals; ++j)
         {
            _readInterval(currentCalAddress, _aiCalInfo[i].intervals[j]);
            currentCalAddress += kIntervalSizeInBytes;
         }
      }
      for (u32 i=0; i<_numberOfDACs; ++i)
      {
         _readMode(currentCalAddress, _aoCalInfo[i].mode);
         currentCalAddress += kModeSizeInBytes;
         for (u32 j=0; j<kNumAOIntervals; ++j)
         {
            _readInterval(currentCalAddress, _aoCalInfo[i].intervals[j]);
            currentCalAddress += kIntervalSizeInBytes;
         }
      }
   }

   void eepromHelper::_readMode(u32 address, tMode& mode)
   {
      mode.order = _readU8(address);
      for (u32 i=0; i<kNumModeCoefficients; ++i)
      {
         mode.coefficients[i] = _readF32(address + sizeof(mode.order) + i*sizeof(f32));
      }
   }

   void eepromHelper::_readInterval(u32 address, tInterval& interval)
   {
      // If it is empty, the interval is invalid.
      interval.valid = _readU32(address) != 0x0;
      if (interval.valid)
      {
         interval.gain = _readF32(address);
         interval.offset = _readF32(address + sizeof(f32));
      }
   }

   u32 eepromHelper::_getOffsetForPages(u32 aOffset, u32 bOffset, nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return 0;

      // Figure out which offset is correct.
      if (aOffset > 0 && bOffset > 0)
      {
         u16 calADataSize = _readU16(aOffset);
         u32 calAWriteCount = _readU32(aOffset + sizeof(u16) + calADataSize);
         u16 selfcalB = _readU16(bOffset);
         u32 selfCalBWriteCount = _readU32(bOffset + sizeof(u16) + selfcalB);

         if (calAWriteCount > selfCalBWriteCount)
         {
            return aOffset;
         }
         else if (selfCalBWriteCount > calAWriteCount)
         {
            return bOffset;
         }
         else
         {
            status.setCode(kStatusBadSelector);
            return 0;
         }
      }
      else if (aOffset > 0)
      {
         return aOffset;
      }
      else if (bOffset > 0)
      {
         return bOffset;
      }
      else
      {
         status.setCode(kStatusBadSelector);
         return 0;
      }
   }

   void eepromHelper::_shiftWindow(u32 address)
   {
      if (_isSimultaneous)
      {
         _baseAddress = (address / nCHInCh::kPageSize) * nCHInCh::kPageSize;
         _xseries.CHInCh.Window_Control_Register.writeRegister(_baseAddress);
      }
   }

   u8 eepromHelper::_readU8(u32 address)
   {
      _shiftWindow(address);
      return _eeprom.read8(address - _baseAddress);
   }

   u16 eepromHelper::_readU16(u32 address)
   {
      _shiftWindow(address);
      return _eeprom.read16(address - _baseAddress);
   }

   u32 eepromHelper::_readU32(u32 address)
   {
      _shiftWindow(address);
      return _eeprom.read32(address - _baseAddress);
   }

   f32 eepromHelper::_readF32(u32 address)
   {
      u8 f32Converter[4];
      // Much of the data in this Cal area is actually an f32.  We need this guy to do cast tricks.
      #if kLittleEndian
      f32Converter[0] = _readU8(address++);
      f32Converter[1] = _readU8(address++);
      f32Converter[2] = _readU8(address++);
      f32Converter[3] = _readU8(address++);
      #else
      f32Converter[3] = _readU8(address++);
      f32Converter[2] = _readU8(address++);
      f32Converter[1] = _readU8(address++);
      f32Converter[0] = _readU8(address++);
      #endif
      return *(reinterpret_cast<f32*>(f32Converter));
   }
} // nNISTC3
