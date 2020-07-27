/*
 * devices.cpp
 *
 * Given bar0 of an X Series device, find the information we care about for it.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include "devices.h"

// Chip Objects
#include "tXSeries.h"

namespace nNISTC3
{
   static const aiRangeGain kAI_MIO_Good10V     = {kInput_10V, 0};
   static const aiRangeGain kAI_MIO_Good5V      = {kInput_5V, 1};
   static const aiRangeGain kAI_MIO_Good1V      = {kInput_1V, 4};
   static const aiRangeGain kAI_MIO_Good200mV   = {kInput_200mV, 5};

   static const aiRangeGain kAI_MIO_Better10V   = {kInput_10V, 1};
   static const aiRangeGain kAI_MIO_Better5V    = {kInput_5V, 2};
   static const aiRangeGain kAI_MIO_Better2V    = {kInput_2V, 3};
   static const aiRangeGain kAI_MIO_Better1V    = {kInput_1V, 4};
   static const aiRangeGain kAI_MIO_Better500mV = {kInput_500mV, 5};
   static const aiRangeGain kAI_MIO_Better200mV = {kInput_200mV, 6};
   static const aiRangeGain kAI_MIO_Better100mV = {kInput_100mV, 7};

   static const aiRangeGain kAI_SMIO_10V  = {kInput_10V, 1};
   static const aiRangeGain kAI_SMIO_5V   = {kInput_5V, 0};
   static const aiRangeGain kAI_SMIO_2V   = {kInput_2V, 2};
   static const aiRangeGain kAI_SMIO_1V   = {kInput_1V, 3};

   static const aiRangeGain * kAI_MIO_GoodMap[] = {
      &kAI_MIO_Good10V,
      &kAI_MIO_Good5V,
      &kAI_MIO_Good1V,
      &kAI_MIO_Good200mV,
      NULL};
   static const aiRangeGain * kAI_MIO_BetterMap[] = {
      &kAI_MIO_Better10V,
      &kAI_MIO_Better5V,
      &kAI_MIO_Better2V,
      &kAI_MIO_Better1V,
      &kAI_MIO_Better500mV,
      &kAI_MIO_Better200mV,
      &kAI_MIO_Better100mV,
      NULL};
   static const aiRangeGain * kAI_SMIO_Map[] = {
      &kAI_SMIO_10V,
      &kAI_SMIO_5V,
      &kAI_SMIO_2V,
      &kAI_SMIO_1V,
      NULL};

   static const aoRangeGain kAO_10V    = {kOutput_10V, 0};
   static const aoRangeGain kAO_5V     = {kOutput_5V, 1};
   static const aoRangeGain kAO_APFI0  = {kOutput_APFI0, 2};
   static const aoRangeGain kAO_APFI1  = {kOutput_APFI1, 3};

   static const aoRangeGain * kAO_NoneMap[] = {NULL};
   static const aoRangeGain * kAO_GoodMap[] = {&kAO_10V, NULL};
   static const aoRangeGain * kAO_Better1ConnectorMap[] = {
      &kAO_10V,
      &kAO_5V,
      &kAO_APFI0,
      NULL};
   static const aoRangeGain * kAO_Better2ConnectorMap[] = {
      &kAO_10V,
      &kAO_5V,
      &kAO_APFI0,
      &kAO_APFI1,
      NULL};

   static const tDeviceInfo kDevices[] = {
      { 0x7425, "PCIe-6320", kFalse, kTrue,  1, 16, kAI_MIO_GoodMap, 0, kAO_NoneMap, 8 },
      { 0x7427, "PCIe-6321", kFalse, kTrue,  1, 16, kAI_MIO_GoodMap, 2, kAO_GoodMap, 8 },
      { 0x7429, "PCIe-6323", kFalse, kTrue,  1, 32, kAI_MIO_GoodMap, 4, kAO_GoodMap, 32 },
      { 0x7428, "PXIe-6323", kFalse, kFalse, 1, 32, kAI_MIO_GoodMap, 4, kAO_GoodMap, 32 },

      { 0x742B, "PCIe-6341", kFalse, kTrue,  1, 16, kAI_MIO_GoodMap, 2, kAO_GoodMap, 8 },
      { 0x742D, "PCIe-6343", kFalse, kTrue,  1, 32, kAI_MIO_GoodMap, 4, kAO_GoodMap, 32 },
      { 0x742A, "PXIe-6341", kFalse, kFalse, 1, 16, kAI_MIO_GoodMap, 2, kAO_GoodMap, 8 },
      { 0x742C, "PXIe-6343", kFalse, kFalse, 1, 32, kAI_MIO_GoodMap, 4, kAO_GoodMap, 32 },

      { 0x742F, "PCIe-6351", kFalse, kTrue,   1, 16, kAI_MIO_BetterMap, 2, kAO_Better1ConnectorMap, 8 },
      { 0x7431, "PCIe-6353", kFalse, kTrue,   1, 32, kAI_MIO_BetterMap, 4, kAO_Better2ConnectorMap, 32 },
      { 0x7436, "PXIe-6356", kTrue,  kFalse,  8,  8, kAI_SMIO_Map, 2, kAO_Better1ConnectorMap, 8 },
      { 0x7437, "PXIe-6358", kTrue,  kFalse, 16, 16, kAI_SMIO_Map, 4, kAO_Better2ConnectorMap, 32 },

      { 0x7433, "PCIe-6361", kFalse, kTrue,   1, 16, kAI_MIO_BetterMap, 2, kAO_Better1ConnectorMap, 8 },
      { 0x7435, "PCIe-6363", kFalse, kTrue,   1, 32, kAI_MIO_BetterMap, 4, kAO_Better2ConnectorMap, 32 },
      { 0x7432, "PXIe-6361", kFalse, kFalse,  1, 16, kAI_MIO_BetterMap, 2, kAO_Better1ConnectorMap, 8 },
      { 0x7434, "PXIe-6363", kFalse, kFalse,  1, 32, kAI_MIO_BetterMap, 4, kAO_Better2ConnectorMap, 32 },
      { 0x7438, "PXIe-6366", kTrue,  kFalse,  8,  8, kAI_SMIO_Map, 2, kAO_Better1ConnectorMap, 8 },
      { 0x7439, "PXIe-6368", kTrue,  kFalse, 16, 16, kAI_SMIO_Map, 4, kAO_Better2ConnectorMap, 32 },

      { 0x74DD, "PXIe-6376", kTrue,  kFalse,  8,  8, kAI_SMIO_Map, 2, kAO_Better1ConnectorMap, 8 },
      { 0x74DE, "PXIe-6378", kTrue,  kFalse, 16, 16, kAI_SMIO_Map, 4, kAO_Better2ConnectorMap, 32 },
   };
   static const u32 kNumberOfDevices = sizeof(kDevices) / sizeof(kDevices[0]);

   u16 tDeviceInfo::getAI_Gain(tInputRange range, nMDBG::tStatus2& status) const
   {
      if (status.isFatal()) return 0;

      u32 i = 0;

      // Cycle through the range/gain pairs in the map, all maps end with a NULL pointer
      while (aiRangeGainMap[i] != NULL)
      {
         if (aiRangeGainMap[i]->range == range)
         {
            // This board supports this range, return the mapped gain
            return aiRangeGainMap[i]->gain;
         }
         ++i;
      }

      // The loop stopped because it reached the end of the list, range not supported
      status.setCode(kStatusBadInterval);
      return 0;
   }

   u8 tDeviceInfo::getAO_Gain(tOutputRange range, nMDBG::tStatus2& status) const
   {
      if (status.isFatal()) return 0;

      u32 i = 0;

      // Cycle through the range/gain pairs in the map, all maps end with a NULL pointer
      while (aoRangeGainMap[i] != NULL)
      {
         if (aoRangeGainMap[i]->range == range)
         {
            // This board supports this range, return the mapped gain
            return aoRangeGainMap[i]->gain;
         }
         ++i;
      }

      // The loop stopped because it reached the end of the list, range not supported
      status.setCode(kStatusBadInterval);
      return 0;
   }

   const tDeviceInfo* getDeviceInfo(tXSeries& xseries, nMDBG::tStatus2& status)
   {
      if (status.isFatal()) return NULL;

      // Read the CHInCh signature register
      u32 CHInChSig = xseries.CHInCh.CHInCh_Identification_Register.readRegister(&status);
      if (CHInChSig != nCHInCh::kCHInChSignature)
      {
         // Device does not have a CHInCh
         status.setCode(kStatusUnknownCHInCh);
      }

      // Read the STC3 signature register
      u32 STC3Signature = xseries.BrdServices.Signature_Register.readRegister(&status);
      if (STC3Signature != nBrdServices::kSTC3_RevASignature && STC3Signature != nBrdServices::kSTC3_RevBSignature)
      {
         // Device does not have an STC3
         status.setCode(kStatusUnknownSTC3);
      }

      u32 productID = xseries.CHInCh.PCI_SubSystem_ID_Access_Register.readSubSystem_Product_ID(&status);

      for (u32 i=0; i<kNumberOfDevices; ++i)
      {
         if (kDevices[i].productID == productID)
         {
            return &kDevices[i];
         }
      }

      status.setCode(kStatusUnknownDevice);
      return NULL;
   }
}
