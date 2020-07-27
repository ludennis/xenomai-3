/*
 * dataHelper.cpp
 *
 * Scale and print device data.
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>

#ifndef ___dataHelper_h___
#include "dataHelper.h"
#endif

#ifndef ___tDIValues_h___
#include "tDIValues.h"
#endif

namespace nNISTC3
{
   void printHeaderPad(const char* label)
   {
      printf("%-12s     ", label);
   }

   void printHeader( u32         numDevices,
                     u32         numChannels,
                     const char* label,
                     const char* subsystem)
   {
      u32 m = 0; // number of channels counter
      u32 n = 0; // number of devices counter

      printHeaderPad(label);
      for (u32 k=0; k<numDevices*numChannels; ++k)
      {
         if (numDevices == 1)
         {
            printf("    %2s%-2u     ", subsystem, m);
            ++m;
         }
         else
         {
            printf("  dev%1u %2s%-2u  ", n, subsystem, m);
            ++m;
            if (m%numChannels == 0)
            {
               ++n;
               m=0;
            }
         }
      }
   }

   namespace nAIDataHelper
   {
      void scaleData( const std::vector<i16>&     rawData,
                      u32                         rawDataLength,
                      std::vector<f32>&           scaledData,
                      u32                         scaledDataLength,
                      const std::vector<nNISTC3::aiHelper::tChannelConfiguration>&  channelConfig,
                      nNISTC3::eepromHelper&      eepromHelper,
                      const nNISTC3::tDeviceInfo& deviceInfo )
      {
         u32 m = 0; // Number of channels counter
         nMDBG::tStatus2 status;

         for (u32 i=0; i<rawDataLength || i<scaledDataLength ; ++i)
         {
            //
            // Get calibration coefficients
            //

            nNISTC3::tAIScalingCoefficients aiCoeffs;
            u32 channel = m%channelConfig.size();
            eepromHelper.getAIScalingCoefficients(deviceInfo.isSimultaneous ? channel : 0, channelConfig[channel].range, aiCoeffs, status);
            ++m;

            scaledData[i] = nNISTC3::scale(rawData[i], aiCoeffs);
         }
      }

      void printHeader( u32          numDevices,
                        u32          numChannels,
                        const char*  label )
      {
         nNISTC3::printHeader(numDevices, numChannels, label, "ai");
      }
   } // nAIDataHelper

   namespace nAODataHelper
   {
      void generateSignal( tSignalType       signalType,
                           f32               minValue,
                           f32               maxValue,
                           std::vector<f32>& signalData,
                           u32               signalDataLength )
      {
         f32 wavePoint = 0;
         f32 amplitude = 1;
         f32 offset = 0;
         for (u32 i=0; i<signalDataLength; ++i)
         {
            switch (signalType)
            {
            case kSineWave:
               wavePoint = static_cast<f32>(sin(2 * M_PI * i / signalDataLength));
               amplitude = (maxValue - minValue) / 2;
               offset = (maxValue + minValue) / 2;
               break;
            case kSquareWave:
               wavePoint = static_cast<f32>(1 - i / (signalDataLength/2));
               amplitude = maxValue - minValue;
               offset = minValue;
               break;
            case kRamp:
               wavePoint = static_cast<f32>(i)/signalDataLength;
               amplitude = maxValue - minValue;
               offset = minValue;
               break;
            default:
               wavePoint = 1;
               break;
            }
            signalData[i] = amplitude * wavePoint + offset;
         }
      }

      void scaleData( const std::vector<f32>& voltData,
                      u32                     voltDataLength,
                      std::vector<i16>&       scaledData,
                      u32                     scaledDataLength,
                      const std::vector<nNISTC3::aoHelper::tChannelConfiguration>&  channelConfig,
                      nNISTC3::eepromHelper&  eepromHelper )
      {
         nMDBG::tStatus2 status;

         u32 numChannels = static_cast<u32>(channelConfig.size());
         for (u32 i=0; i<voltDataLength || i<scaledDataLength; i+=numChannels)
         {
            for (u32 m=0; m<numChannels; ++m)
            {
               // Get AO scaling coefficients for this channel
               nNISTC3::tAOScalingCoefficients aoCoeffs;
               eepromHelper.getAOScalingCoefficients(channelConfig[m].channel, channelConfig[m].range, aoCoeffs, status);

               scaledData[i+m] = scale(voltData[i+m], aoCoeffs);
            }
         }
      }

      void printHeader( u32         numDevices,
                        u32         numChannels,
                        const char* label )
      {
         nNISTC3::printHeader(numDevices, numChannels, label, "ao");
      }
   } // nAODataHelper

   namespace nDIODataHelper
   {
      void printHeader(u32 port)
      {
         nNISTC3::printHeaderPad("Digital");
         printf("    port %1u", port);
         printf("\n");
      }

      void printData( const std::vector<u8> data,
                      u32                   dataLength,
                      u32                   sampleSize )
      {
         static u32 n = 0;  // Number of samples counter

         for (u32 i=0; i<dataLength; i+=sampleSize)
         {
            ++n;
            if (sampleSize == 1)
            {
               u8 value = data[i];
               printf("%8s %6d:      0x%02hX\n", "Sample", n, value & nDI::nDI_FIFO_Data_Register8::nCDI_FIFO_Data8::kMask);
            }
            else
            {
               tFourByteInt_t value;
               value.byte[0] = data[i+0];
               value.byte[1] = data[i+1];
               value.byte[2] = data[i+2];
               value.byte[3] = data[i+3];

               printf("%8s %6d:   0x%08X\n", "Sample", n, value.value & nDI::nDI_FIFO_Data_Register::nCDI_FIFO_Data::kMask);
            }
         }
      }
   } // nDIODataHelper

   namespace nGPCTDataHelper
   {
      void scaleData( const std::vector<u32>&   rawData,
                      u32                       rawDataLength,
                      std::vector<f32>&         scaledData,
                      u32                       scaledDataLength,
                      const tFrequencyScaler_t& scaler )
      {
         u32 gateCount = 0;
         u32 sourceCount = 0;
         f32 scaled = 0;
         nMDBG::tStatus2 status;

         for (u32 i=0, j=0; i<rawDataLength || j<scaledDataLength ; ++j)
         {
            if (scaler.isAveraged)
            {
               gateCount = rawData[i];
               sourceCount = rawData[i+1];
               scaled = scaler.timebaseRate * static_cast<f32>(gateCount) / static_cast<f32>(sourceCount);
               i+=2;
            }
            else
            {
               sourceCount = rawData[i];
               scaled = scaler.timebaseRate / static_cast<f32>(sourceCount);
               ++i;
            }
            scaledData[j] = scaled;
         }
      }

      void scaleData( const std::vector<u32>& rawData,
                      u32                     rawDataLength,
                      std::vector<f32>&       scaledData,
                      u32                     scaledDataLength,
                      const tEncoderScaler_t& scaler )
      {
         i32 count = 0;
         f32 scaled = 0;

         for (u32 i=0, j=0; i<rawDataLength || j<scaledDataLength ; ++i, ++j)
         {
            count = static_cast<i32>(rawData[i]);

            if (scaler.isAngular)
            {
               scaled = count*360/static_cast<f32>(scaler.pulsesPerRevolution);
            }
            else
            {
               scaled = count*static_cast<f32>(scaler.distancePerPulse);
            }
            scaledData[j] = scaled;
         }
      }

      void scaleData( const std::vector<u32>&     rawData,
                      u32                         rawDataLength,
                      std::vector<f32>&           scaledData,
                      u32                         scaledDataLength,
                      const tDigWaveformScaler_t& scaler )
      {
         for (u32 i=0; i<rawDataLength || i<scaledDataLength; ++i)
         {
            scaledData[i] = rawData[i] / scaler.timebaseRate;
         }
      }

      void scaleData( const std::vector<f32>&    timeData,
                      u32                        timeDataLength,
                      std::vector<u32>&          scaledData,
                      u32                        scaledDataLength,
                      const tPulseTrainScaler_t& scaler )
      {
         u32 count;
         for (u32 i=0; i<timeDataLength || i<scaledDataLength; ++i)
         {
            count = static_cast<u32>(timeData[i] * scaler.timebaseRate);
            scaledData[i] = count - 1; // Subtract one for counter's terminal count
         }
      }

      void printHeader( const char* label,
                        const char* units )
      {
         nNISTC3::printHeaderPad(label);
         printf("%s", units);
      }
   } // nGPCTDataHelper
} // nNISTC3
