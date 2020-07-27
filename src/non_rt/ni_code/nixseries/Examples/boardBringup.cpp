/*
 * boardBringup.cpp
 *   Self test an X Series device
 *
 * boardBringup performs a self test on an X Series device. After identifying
 * the X Series device, boardBringup reads the signature registers and prints
 * other hardware information, including the device's on-board temperature.
 * Next, boardBringup reads the device's EEPROM and prints the calibration
 * meta-information and scaling coefficients. Finally, boardBringup tests
 * device register IO by writing to and reading from the scratch pad registers.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 * ! Operation       : self test
 * ! Register IO     : read signature registers, test scratch pad registers
 * ! EEPROM          : read calibration information and coefficients
 * Channel
 * ! Channels        : internal temperature sensor
 *
 * External Connections
 *   None
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include <math.h> // for fabs()
#include <numeric> // for accumulate()
#include <stdio.h>
#include <time.h>
#include <vector>

// OS Interface
#include "osiBus.h"

// Chip Objects
#include "tXSeries.h"

// Chip Object Helpers
#include "dataHelper.h"
#include "devices.h"
#include "eepromHelper.h"
#include "inTimer/aiHelper.h"
#include "simultaneousInit.h"

f32 getCurrentTemp(const nNISTC3::tDeviceInfo&, tXSeries&, nNISTC3::eepromHelper&, nMDBG::tStatus2&);

void test(iBus* bus)
{
   /*********************************************************************\
   |
   |   Example parameters
   |
   \*********************************************************************/

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Device parameters
   u32 CHInChSig = 0;
   u32 STC3Signature = 0;
   u32 STC3_Revision = 0;
   u32 serialNumber = 0;
   u32 geographicalAddress = 0;

   f32 currentTemp = 0;
   f32 extCalTemp = 0;
   f32 selfCalTemp = 0;
   time_t rawExtCalTime, rawSelfCalTime;

   nNISTC3::tAIScalingCoefficients aiCoefficients;
   nNISTC3::tAOScalingCoefficients aoCoefficients;

   tBoolean selfTestPassed = kTrue;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tAddressSpace bar0;
   bar0 = bus->createAddressSpace(kPCI_BAR0);

   /*********************************************************************\
   |
   |   Initialize the self test
   |
   \*********************************************************************/

   //
   // Open, detect, and initialize the X Series device
   //

   tXSeries device(bar0, &status);

   const nNISTC3::tDeviceInfo* deviceInfo = nNISTC3::getDeviceInfo(device, status);
   if (status.isFatal())
   {
      printf("Error: Cannot identify device (%d).\n", status.statusCode);
      return;
   }

   if (deviceInfo->isSimultaneous) nNISTC3::initializeSimultaneousXSeries(device, status);

   printf("Device Information\n");
   printf("%-24s: %s\n%-24s: 0x%04X\n%-24s: %s\n%-24s: %u\n%-24s: %u\n",
      "  Device", deviceInfo->deviceName,
      "  Product ID", deviceInfo->productID,
      "  Is simultaneous", deviceInfo->isSimultaneous ? "Yes" : "No",
      "  Number of ADCs", deviceInfo->numberOfADCs,
      "  Number of DACs", deviceInfo->numberOfDACs);

   //
   // Create subsystem helpers
   //

   nNISTC3::eepromHelper eeprom(device, deviceInfo->isSimultaneous, deviceInfo->numberOfADCs, deviceInfo->numberOfDACs, status);

   //
   // Validate the Feature Parameters
   //

   // Nothing else to validate

   /*********************************************************************\
   |
   |   Perform the self test
   |
   \*********************************************************************/

   //
   // Read the CHInCh signature register
   //

   CHInChSig = device.CHInCh.CHInCh_Identification_Register.readRegister(&status);
   printf("%-24s: 0x%08X\n", "  CHInCh signature", CHInChSig);

   //
   // Read the STC3 signature register
   //

   STC3Signature = device.BrdServices.Signature_Register.readRegister(&status);
   printf("%-24s: 0x%08X\n", "  STC3 signature", STC3Signature);

   //
   // Read the Simultaneous Control signature register
   //

   if (deviceInfo->isSimultaneous)
   {
      printf("%-24s: 0x%02X%02X%02X%02X\n",
         "  Simultaneous sig",
         device.SimultaneousControl.SignatureYear.readRegister(&status),
         device.SimultaneousControl.SignatureMonth.readRegister(&status),
         device.SimultaneousControl.SignatureDay.readRegister(&status),
         device.SimultaneousControl.SignatureHour.readRegister(&status));
   }

   //
   // Read the device's EEPROM
   //

   STC3_Revision = eeprom.getSTC3_Revision();
   serialNumber = eeprom.getSerialNumber();
   geographicalAddress = eeprom.getGeographicalAddress();
   extCalTemp = eeprom.getExtCalTemp();
   selfCalTemp = eeprom.getSelfCalTemp();
   rawExtCalTime = eeprom.getExtCalTime();
   rawSelfCalTime = eeprom.getSelfCalTime();

   printf("%-24s: %s\n", "  STC3 revision", (STC3_Revision == 1) ? "A" : "B");
   printf("%-24s: 0x%08X\n", "  Serial number", serialNumber);
   printf("%-24s: %d\n", "  Slot number", geographicalAddress);

   currentTemp = getCurrentTemp(*deviceInfo, device, eeprom, status);
   if (status.isFatal())
   {
      printf("Error: Could not read device temperature.\n");
      return;
   }
   printf("%-24s: %.3f C\n", "  Board temperature", currentTemp);

   printf("\n");
   printf("Calibration Information\n");
   printf("  External Calibration\n");
   printf("%-24s: %s", "    Cal time", asctime(localtime(&rawExtCalTime)));
   printf("%-24s: %.3f\n", "    Voltage reference", eeprom.getExtCalVoltRef());
   printf("%-24s: %.3f C\n", "    Cal temperature", extCalTemp);
   printf("%-24s: %.3f C\n", "    Temperature drift", fabs(extCalTemp-currentTemp));

   printf("  Self Calibration\n");
   printf("%-24s: %s", "    Cal time", asctime(localtime(&rawSelfCalTime)));
   printf("%-24s: %.3f\n", "    Voltage reference", eeprom.getSelfCalVoltRef());
   printf("%-24s: %.3f C\n", "    Cal temperature", selfCalTemp);
   printf("%-24s: %.3f C\n", "    Temperature drift", fabs(selfCalTemp-currentTemp));

   printf("\n");
   printf("Scaling Information\n");
   for (u32 i=0; i<deviceInfo->numberOfADCs; ++i)
   {
      for (u32 j=0; j<nNISTC3::eepromHelper::kNumAIIntervals; ++j)
      {
         nMDBG::tStatus2 tempStatus;
         eeprom.getAIScalingCoefficients(i, j, aiCoefficients, tempStatus);
         if (tempStatus.statusCode == kStatusBadInterval) continue;

         printf("  ADC%u: Interval %u\n    Coefficients: ", i, j);
         for (u32 k=0; k<nNISTC3::kNumAIScalingCoefficients; ++k)
         {
            printf("%.3e ", aiCoefficients.c[k]);
         }
         printf("\n");
         printf("    ADC code 0x0000 is %.3e V\n\n", nNISTC3::scale(0, aiCoefficients));
      }
   }
   for (u32 i=0; i<deviceInfo->numberOfDACs; ++i)
   {
      for (u32 j=0; j<nNISTC3::eepromHelper::kNumAOIntervals; ++j)
      {
         nMDBG::tStatus2 tempStatus;
         eeprom.getAOScalingCoefficients(i, j, aoCoefficients, tempStatus);
         if (tempStatus.statusCode == kStatusBadInterval) continue;

         printf("  DAC%u: Interval %u\n    Coefficients: ", i, j);
         for (u32 k=0; k<nNISTC3::kNumAOScalingCoefficients; ++k)
         {
            printf("%.3e ", aoCoefficients.c[k]);
         }
         printf("\n");
         printf("    0 V is DAC code 0x%04.4hX\n\n", nNISTC3::scale(0, aoCoefficients));
      }
   }

   //
   // Test device communication
   //

   // Write rolling ones to the scratch pad of the CHInCh
   for (u32 i=0; i<32 && status.isNotFatal(); ++i)
   {
      u32 valueToWrite = 1 << i;
      device.CHInCh.Scrap_Register[0].writeRegister(valueToWrite, &status);
      u32 valueRead = device.CHInCh.Scrap_Register[0].readRegister(&status);
      if (valueToWrite != valueRead) selfTestPassed = kFalse;
   }

   // Write rolling zeros to the scratch pad of the CHInCh
   for (u32 i=0; i<32 && status.isNotFatal(); ++i)
   {
      u32 valueToWrite = ~(1 << i);
      device.CHInCh.Scrap_Register[0].writeRegister(valueToWrite, &status);
      u32 valueRead = device.CHInCh.Scrap_Register[0].readRegister(&status);
      if (valueToWrite != valueRead) selfTestPassed = kFalse;
   }

   // Write rolling ones to the scratch pad of the STC3
   for (u32 i=0; i<32 && status.isNotFatal(); ++i)
   {
      u32 valueToWrite = 1 << i;
      device.BrdServices.ScratchPadRegister.writeRegister(valueToWrite, &status);
      u32 valueRead = device.BrdServices.ScratchPadRegister.readRegister(&status);
      if (valueToWrite != valueRead) selfTestPassed = kFalse;
   }

   // Write rolling zeros to the scratch pad of the STC3
   for (u32 i=0; i<32 && status.isNotFatal(); ++i)
   {
      u32 valueToWrite = ~(1 << i);
      device.BrdServices.ScratchPadRegister.writeRegister(valueToWrite, &status);
      u32 valueRead = device.BrdServices.ScratchPadRegister.readRegister(&status);
      if (valueToWrite != valueRead) selfTestPassed = kFalse;
   }

   if (deviceInfo->isSimultaneous)
   {
      // Write rolling ones to the scratch pad of the Simultaneous Control
      for (u32 i=0; i<8 && status.isNotFatal(); ++i)
      {
         u8 valueToWrite = 1 << i;
         device.SimultaneousControl.Scratch.writeRegister(valueToWrite, &status);
         u8 valueRead = device.SimultaneousControl.Scratch.readRegister(&status);
         if (valueToWrite != valueRead) selfTestPassed = kFalse;
      }

      // Write rolling zeros to the scratch pad of the Simultaneous Control
      for (u32 i=0; i<8 && status.isNotFatal(); ++i)
      {
         u8 valueToWrite = ~(1 << i);
         device.SimultaneousControl.Scratch.writeRegister(valueToWrite, &status);
         u8 valueRead = device.SimultaneousControl.Scratch.readRegister(&status);
         if (valueToWrite != valueRead) selfTestPassed = kFalse;
      }
   }

   if (selfTestPassed && status.isNotFatal())
   {
      printf("Device self test PASSED.\n");
   }
   else
   {
      printf("Device self test FAILED.\n");
      status.setCode(kStatusRuntimeError);
   }
}

f32 getCurrentTemp(const nNISTC3::tDeviceInfo& deviceInfo, tXSeries& device, nNISTC3::eepromHelper& eeprom, nMDBG::tStatus2& status)
{
   nNIOSINT100_mUnused(device);

   if (deviceInfo.isSimultaneous)
   {
      device.SimultaneousControl.TempSensorCtrlStat.writeTempStart(kTrue, &status);

      while (!device.SimultaneousControl.TempSensorCtrlStat.readTempReady(&status)) {}

      i32 value =
         (device.SimultaneousControl.TempSensorDataHi.readTempUpperByte(&status) << 8) |
          device.SimultaneousControl.TempSensorDataLo.readTempLowerByte(&status);

      // 10 bits but not properly sign-extended. Do that now.
      if (value & 0x00000200)
      {
         value |= 0xFFFFFE00;
      }

      return static_cast<f32>(value * 0.25);
   }
   else
   {
      f64 rawTempVoltage = 0.0;
      // For specifics on programming an AI measurement, refer to the AI examples.
      {
         u32 numSamps = 256;
         std::vector<i16> rawData(numSamps, 0);
         std::vector<f32> scaledData(numSamps, 0);

         nNISTC3::aiHelper aiHelper(device, deviceInfo.isSimultaneous, status);
         aiHelper.reset(status);

         device.AI.AI_Timer.Reset_Register.writeConfiguration_Start(kTrue, &status);
            aiHelper.programExternalGate(nAI::kGate_Disabled, nAI::kActive_High_Or_Rising_Edge, status);
            aiHelper.programStart1(nAI::kStart1_SW_Pulse, nAI::kActive_High_Or_Rising_Edge, kTrue, status);
            aiHelper.programStart(nAI::kStartCnv_InternalTiming, nAI::kActive_High_Or_Rising_Edge, kTrue, status);
            aiHelper.programConvert(nAI::kStartCnv_InternalTiming, nAI::kActive_Low_Or_Falling_Edge, status);

            nNISTC3::inTimerParams timingConfig;
            timingConfig.setAcqLevelTimingMode(nNISTC3::kInTimerPostTrigger, status);
            timingConfig.setUseSICounter(kTrue, status);
            timingConfig.setSamplePeriod(500, status);
            timingConfig.setSampleDelay(2, status);
            timingConfig.setNumberOfSamples(numSamps, status);
            timingConfig.setUseSI2Counter(kTrue, status);
            timingConfig.setConvertPeriod(500, status);
            timingConfig.setConvertDelay(2, status);
            aiHelper.getInTimerHelper(status).programTiming(timingConfig, status);

            aiHelper.programFIFOWidth(nAI::kTwoByteFifo, status);
         device.AI.AI_Timer.Reset_Register.writeConfiguration_End(kTrue, &status);

         aiHelper.getInTimerHelper(status).clearConfigurationMemory(status);
         std::vector<nNISTC3::aiHelper::tChannelConfiguration> chanConfig(1, nNISTC3::aiHelper::tChannelConfiguration());
         chanConfig[0].isLastChannel = kTrue;
         chanConfig[0].enableDither = nAI::kEnabled;
         chanConfig[0].gain = deviceInfo.getAI_Gain(nNISTC3::kInput_10V, status);
         chanConfig[0].range = nNISTC3::kInput_10V;
         chanConfig[0].channelType = nAI::kInternal;
         chanConfig[0].bank = nAI::kBank0;
         chanConfig[0].channel = 4;
         aiHelper.programChannel(chanConfig[0], status);
         aiHelper.getInTimerHelper(status).primeConfigFifo(status);

         aiHelper.getInTimerHelper(status).armTiming(timingConfig, status);
         aiHelper.getInTimerHelper(status).strobeStart1(status);

         while (!device.AI.AI_Timer.Status_1_Register.readSC_TC_St(&status)) {}
         while (device.AI.AI_Data_FIFO_Status_Register.readRegister(&status) != numSamps) {}

         for (u32 i=0; i<numSamps; ++i)
         {
            rawData[i] = device.AI.AI_FIFO_Data_Register16.readRegister();
         }
         nNISTC3::nAIDataHelper::scaleData(rawData, numSamps,
                                           scaledData, numSamps,
                                           chanConfig, eeprom, deviceInfo);
         rawTempVoltage = std::accumulate(scaledData.begin(), scaledData.end(), 0.0);
         rawTempVoltage /= numSamps;

         aiHelper.reset(status);
      }

      return static_cast<f32>(((-1*rawTempVoltage)-0.5) * 100);
   }
}
