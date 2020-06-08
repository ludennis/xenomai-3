/*
 * simultaneousInit.cpp
 *
 * Initializes simultaneous X Series boards.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include "simultaneousInit.h"

// Chip Objects
#include "tXSeries.h"

namespace nNISTC3
{
   void initializeSimultaneousXSeries(tXSeries& xseries, nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return;

      // Enable EEPROM and Simultaneous Control
      u32 configRegisterValue = xseries.CHInCh.Configuration_Register.readRegister(&status);
      configRegisterValue = (configRegisterValue & ~nCHInCh::kEEPROMConfigMask) | nCHInCh::kEEPROMConfig;
      configRegisterValue = (configRegisterValue & ~nCHInCh::kSMIOConfigMask) | nCHInCh::kSMIOConfig;
      xseries.CHInCh.Configuration_Register.writeRegister(configRegisterValue, &status);

      // Configure EEPROM
      xseries.CHInCh.EEPROM_Register_0.writeRegister(nCHInCh::kEEPROMSettingsRegister0Value, &status);
      xseries.CHInCh.EEPROM_Register_1.writeRegister(nCHInCh::kEEPROMSettingsRegister1Value, &status);
      xseries.CHInCh.EEPROM_Register_2.writeRegister(nCHInCh::kEEPROMSettingsRegister2Value, &status);

      // Configure Simultaneous Control
      xseries.CHInCh.SMIO_Register_0.writeRegister(nCHInCh::kSimultaneousRegister0Value, &status);
      xseries.CHInCh.SMIO_Register_1.writeRegister(nCHInCh::kSimultaneousRegister1Value, &status);
      xseries.CHInCh.SMIO_Register_2.writeRegister(nCHInCh::kSimultaneousRegister2Value, &status);
      xseries.CHInCh.SMIO_Register_3.writeRegister(nCHInCh::kSimultaneousRegister3Value, &status);

      // Configure Windows so that we can read them through the CHInCh
      xseries.CHInCh.EEPROM_Window_Register.writeRegister(nCHInCh::kEEPROMOffset | nCHInCh::kWindowEnable | nCHInCh::kWindowSize, &status);
      xseries.CHInCh.Simultaneous_Window_Register.writeRegister(nCHInCh::kSMIOOffset | nCHInCh::kWindowEnable | nCHInCh::kWindowSize, &status);

      // Configure AI
      tBusSpaceReference bar0 = xseries.getBusSpaceReference();
      bar0.write16(kSimultaneousAIConfiguration0Address, kSimultaneousAIConfiguration0Value);
      bar0.write16(kSimultaneousAIConfiguration1Address, kSimultaneousAIConfiguration1Value);
   }
}
