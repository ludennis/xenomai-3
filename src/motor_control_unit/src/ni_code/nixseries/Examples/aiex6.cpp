/*
 * aiex6.cpp
 *   Multi-device continuous hardware-timed analog input with optimized DMA
 *
 * aiex6 reads and scales analog data from two devices using hardware timing
 * and transfers data to the host using optimized DMA buffers. Before
 * configuring the AI subsystem on each device, aiex6 programs the devices'
 * Phase-Lock Loop circuits to lock to a common reference clock and uses
 * backplane (for PXIe) or RTSI (for PCIe) signals to achieve nanosecond
 * synchronization with trigger skew correction. In addition, aiex6 exports
 * both devices' sample clocks on PFI0 to show their phase-alignment. After
 * the AI subsystem's timing and channel parameters have been programmed,
 * aiex6 configures and starts the DMA channel before waiting 10 seconds for
 * an external start trigger. Once triggered, data is read, scaled, and
 * printed in chunks for 10 seconds until aiex6 stops the timing engine,
 * shuts down DMA, and flushes the buffer. Finally, aiex6 restores the
 * hardware's previous state.
 *
 * Example Features (! means highlighted, * means default)
 * Device
 *   Operation       : voltage measurement
 * ! Synchronization : Phase-Lock Loop (PLL) (*) or shared start trigger only
 * ! PLL source      : 100 MHz PXIe backplane clock for PXIe devices
 *                      10 MHz on-board reference clock for PCIe devices
 * Channel
 *   Channels        : ai0, ai1 on both devices
 *   Scaling         : Volts (*) or raw ADC codes
 *   Terminal config : RSE (*), differential, non-referenced single-ended
 *   Input range     : +/- 10 V (*),  +/- 5 V,     +/- 2 V,    +/- 1 V,
 *                     +/- 500mV,     +/- 200 mV,  +/- 100 mV
 * Timing
 *   Sample mode     : continuous
 *   Timing mode     : hardware-timed
 *   Clock source    : on-board oscillator
 *   Clock rate      : 5 kHz
 * Trigger
 * ! Start trigger   : Master: PFI1 (digital rising edge)
 * !                   Slave: from master device via backplane
 *   Reference trig  : none
 *   Pause trigger   : none
 * Read Buffer
 *   Data transfer   : optimized scatter-gather DMA ring
 *   DMA transfer    : the buffer is optimized to maximize throughput
 *   DMA buffer      : overwrite or preserve (*) unread samples
 * Behavior
 *   Runtime         : 10 seconds
 *   Trig wait time  : 10 seconds
 *
 * External Connections
 *   ai0:1           : voltages within +/- 10 V (*) or other specified range
 *   Master PFI1     : TTL trigger
 *   If PCIe         : connect the two devices with a RTSI cable
 *   Master PFI0     : observe on an oscilloscope
 *   Slave  PFI0     : observe on an oscilloscope
 *
 * Copyright 2011 National Instruments
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#include <memory> // For auto_ptr<T>
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
#include "pfiRtsiResetHelper.h"
#include "pllHelper.h"
#include "simultaneousInit.h"
#include "streamHelper.h"

// DMA Support
#include "CHInCh/dmaErrors.h"
#include "CHInCh/dmaProperties.h"
#include "CHInCh/tCHInChDMAChannel.h"

void test(iBus* bus)
{
   /*********************************************************************\
   |
   |   Example parameters
   |
   \*********************************************************************/

   //
   // Feature Parameters (modify these to exercise the example)
   //

   // Device parameters
   tChar* slaveDeviceLocation = "PXI12::0::INSTR";
   tBoolean synchronizeDevices = kTrue;

   // Channel parameters
   const u32 numberOfChannels = 2;                               // Channels: ai0:1
   tBoolean printVolts = kTrue;
   const nNISTC3::tInputRange range = nNISTC3::kInput_10V;       // Range: +/- 10 Volts
   const nAI::tAI_Config_Channel_Type_t termConfig = nAI::kRSE;  // Term config: Single-ended (aiN vs aiGnd)

   // Timing parameters
   u32 samplePeriod = 20000;                               // Rate: 100 MHz / 20e3 = 5 kHz

   // Trigger parameters
   const nAI::tAI_START1_Select_t masterTrigger = nAI::kStart1_PFI1; // The master device receives an external trigger signal

   // Buffer parameters
   const u32 sampsPerChan = 512;
   const u32 dmaBufferFactor = 16;
   tBoolean allowOverwrite = kFalse;

   // Behavior parameters
   const f64 runTime = 10;
   const f64 triggerWaitTime = 10;

   //
   // Fixed or calculated parameters (do not modify these)
   //

   // Device parameters
   const u32 numberOfDevices = 2;
   iBus* slaveBus = acquireBoard(slaveDeviceLocation);
   if (slaveBus == NULL)
   {
      printf("Error: Could not access slave PCI device at %s.\n", slaveDeviceLocation);
      return;
   }

   nTriggers::tTrig_PLL_In_Source_Select_t desiredPLL_Source = nTriggers::kRefClkSrc_PXIe_Clk100;
   nNISTC3::PLL_Parameters_t parameters;
   parameters.pllDivisor = 1;             // Divisor is always 1
   parameters.pllOutputDivider = 8;       // Output divider is always 8

   // Channel parameters
   u16 masterGain = 0;
   u16 slaveGain = 0;
   std::vector<nNISTC3::aiHelper::tChannelConfiguration> masterChanConfig(numberOfChannels, nNISTC3::aiHelper::tChannelConfiguration());
   std::vector<nNISTC3::aiHelper::tChannelConfiguration> slaveChanConfig(numberOfChannels, nNISTC3::aiHelper::tChannelConfiguration());
   u32 maxChannel = numberOfChannels;

   // Timing parameters
   const nAI::tAI_StartConvertSelMux_t sampClkSrc = nAI::kStartCnv_InternalTiming; // On-board clock
   const nAI::tAI_Polarity_t sampClkPolarity = nAI::kActive_High_Or_Rising_Edge;   // Clock on rising edge or level
   const u32 sampleDelay = 2; // Wait N TB3 ticks after the start trigger before clocking (N must be >= 2)
   const u32 convertPeriod = 400; // 100 MHz / 400 = 250 kHz (refer to device specs for minimum period)
   const u32 convertDelay = 2; // Wait N TB3 ticks after the sample clock before converting (N must be >= 2)
   nNISTC3::inTimerParams masterTimingConfig;
   nNISTC3::inTimerParams slaveTimingConfig;

   // Trigger parameters
   const nAI::tAI_START1_Select_t slaveTrigger = nAI::kStart1_RTSI0; // The slave receives the master's exported trigger on the backplane
   const nAI::tAI_Polarity_t triggerPolarity = nAI::kActive_High_Or_Rising_Edge;

   // Buffer parameters
   tBoolean dataOverwritten = kFalse; // Have the DMA channels overwritten any unread data?
   u32 bytesRead = 0;            // Bytes read so far from both buffers
   u32 masterBytesAvailable = 0; // Bytes in the master DMA buffer
   u32 slaveBytesAvailable = 0;  // Bytes in the slave DMA buffer
   const nNISTC3::tDMAChannelNumber dmaChannel = nNISTC3::kAI_DMAChannel;

   const u32 sampleSizeInBytes = sizeof(i16);
   const u32 totalNumberOfSamples = numberOfChannels * sampsPerChan;
   u32 readSizeInBytes = totalNumberOfSamples * sampleSizeInBytes;
   const u32 dmaSizeInBytes = dmaBufferFactor * readSizeInBytes;

   std::vector<i16> masterRawData(totalNumberOfSamples, 0);
   std::vector<i16> slaveRawData(totalNumberOfSamples, 0);
   std::vector<i16> jointRawData;
   std::vector<f32> masterScaledData(totalNumberOfSamples, 0);
   std::vector<f32> slaveScaledData(totalNumberOfSamples, 0);
   std::vector<f32> jointScaledData;

   // Behavior parameters
   const tBoolean holdPfiRtsiState = kFalse;
   f64 elapsedTime = 0; // How long has the measurement been running?
   clock_t start;
   const f64 rlpTimeout = 5; // Wait 5 seconds for a register operation before breaking
   clock_t rlpStart;
   f64 rlpElapsedTime;

   // Bookkeeping
   nMDBG::tStatus2 status;
   tBoolean masterAiErrored = kFalse; // Did the master's AI subsystem have an error?
   nInTimer::tInTimer_Error_t masterScanOverrun = nInTimer::kNO_ERROR;
   nInTimer::tInTimer_Error_t masterAdcOverrun = nInTimer::kNO_ERROR;
   nInTimer::tInTimer_Error_t masterFifoOverflow = nInTimer::kNO_ERROR;
   tBoolean slaveAiErrored = kFalse; // Did the slave's AI subsystem have an error?
   nInTimer::tInTimer_Error_t slaveScanOverrun = nInTimer::kNO_ERROR;
   nInTimer::tInTimer_Error_t slaveAdcOverrun = nInTimer::kNO_ERROR;
   nInTimer::tInTimer_Error_t slaveFifoOverflow = nInTimer::kNO_ERROR;
   tBoolean dmaErrored = kFalse; // Did the DMA channels have an error?
   nMDBG::tStatus2 dmaErr;
   tAddressSpace masterBar0;
   masterBar0 = bus->createAddressSpace(kPCI_BAR0);
   tAddressSpace slaveBar0;
   slaveBar0 = slaveBus->createAddressSpace(kPCI_BAR0);

   /*********************************************************************\
   |
   |   Initialize the measurement
   |
   \*********************************************************************/

   //
   // Open, detect, and initialize the X Series devices
   //

   tXSeries masterDevice(masterBar0, &status);

   const nNISTC3::tDeviceInfo* masterDeviceInfo = nNISTC3::getDeviceInfo(masterDevice, status);
   if (status.isFatal())
   {
      printf("Error: Cannot identify master device (%d).\n", status.statusCode);
      return;
   }

   if (masterDeviceInfo->isSimultaneous) nNISTC3::initializeSimultaneousXSeries(masterDevice, status);

   tXSeries slaveDevice(slaveBar0, &status);

   const nNISTC3::tDeviceInfo* slaveDeviceInfo = nNISTC3::getDeviceInfo(slaveDevice, status);
   if (status.isFatal())
   {
      printf("Error: Cannot identify slave device (%d).\n", status.statusCode);
      return;
   }

   if (slaveDeviceInfo->isSimultaneous) nNISTC3::initializeSimultaneousXSeries(slaveDevice, status);

   //
   // Create subsystem helpers
   //

   // Since the RTSI lines are used to route timing signals to the PLL, it is
   // important for their settings to remain intact while the PLL is enabled.
   // Create the pfiRtsiResetHelpers first to ensure their destructors run
   // after the pllHelpers' destructors.
   nNISTC3::pfiRtsiResetHelper masterPfiRtsiResetHelper(masterDevice.Triggers, holdPfiRtsiState, status);
   nNISTC3::pfiRtsiResetHelper slavePfiRtsiResetHelper(slaveDevice.Triggers, holdPfiRtsiState, status);

   nNISTC3::pllHelper masterPllHelper(masterDevice, masterDeviceInfo->isPCIe, status);
   nNISTC3::pllHelper slavePllHelper(slaveDevice, slaveDeviceInfo->isPCIe, status);

   nNISTC3::aiHelper masterAiHelper(masterDevice, masterDeviceInfo->isSimultaneous, status);
   nNISTC3::eepromHelper masterEepromHelper(masterDevice, masterDeviceInfo->isSimultaneous, masterDeviceInfo->numberOfADCs, masterDeviceInfo->numberOfDACs, status);
   nNISTC3::streamHelper masterStreamHelper(masterDevice.AIStreamCircuit, masterDevice.CHInCh, status);

   nNISTC3::aiHelper slaveAiHelper(slaveDevice, slaveDeviceInfo->isSimultaneous, status);
   nNISTC3::eepromHelper slaveEepromHelper(slaveDevice, slaveDeviceInfo->isSimultaneous, slaveDeviceInfo->numberOfADCs, slaveDeviceInfo->numberOfDACs, status);
   nNISTC3::streamHelper slaveStreamHelper(slaveDevice.AIStreamCircuit, slaveDevice.CHInCh, status);

   //
   // Validate the Feature Parameters
   //

   // Determine which device has the fewest number of channels
   if (masterDeviceInfo->numberOfAIChannels > slaveDeviceInfo->numberOfAIChannels)
   {
      maxChannel = slaveDeviceInfo->numberOfAIChannels;
   }
   else
   {
      maxChannel = masterDeviceInfo->numberOfAIChannels;
   }

   // Ensure that the number of channels match the number requested
   if (numberOfChannels > masterDeviceInfo->numberOfAIChannels)
   {
      if (masterDeviceInfo->isSimultaneous)
      {
         printf("Error: Number of channels requested (%u) is greater than number on the master device (%u).\n",
            numberOfChannels, masterDeviceInfo->numberOfAIChannels);
         status.setCode(kStatusBadParameter);
         return;
      }
   }

   if (numberOfChannels > slaveDeviceInfo->numberOfAIChannels)
   {
      if (slaveDeviceInfo->isSimultaneous)
      {
         printf("Error: Number of channels requested (%u) is greater than number on the slave device (%u).\n",
            numberOfChannels, slaveDeviceInfo->numberOfAIChannels);
         status.setCode(kStatusBadParameter);
         return;
      }
   }

   // Ensure that the range is supported by these boards
   masterGain = masterDeviceInfo->getAI_Gain(range, status);
   if (status.isFatal())
   {
      printf("Error: Unsupported master range.\n");
      status.setCode(kStatusBadParameter);
      return;
   }

   slaveGain = slaveDeviceInfo->getAI_Gain(range, status);
   if (status.isFatal())
   {
      printf("Error: Unsupported slave range.\n");
      status.setCode(kStatusBadParameter);
      return;
   }

   // Coerce the the sample clock if it will overrun MIO devices
   if ( (!masterDeviceInfo->isSimultaneous || !slaveDeviceInfo->isSimultaneous) && (samplePeriod < numberOfChannels*convertPeriod) )
   {
      printf("Warning: Coercing sample period from %u ticks to %u ticks to meet sample clock requirements.\n", samplePeriod, numberOfChannels*convertPeriod);
      samplePeriod = numberOfChannels*convertPeriod;
   }

   /*********************************************************************\
   |
   |   Program peripheral subsystems
   |
   \*********************************************************************/

   //
   // Configure and route PFI lines
   //

   masterDevice.Triggers.PFI_Direction_Register.writePFI1_Pin_Dir(nTriggers::kPFI_Input, &status); // External start trigger

   // Connect the master device's trigger to the slave device
   // 1. Export the master trigger on PXI_Trig0 (or RTSI0)
   masterDevice.Triggers.RTSI_OutputSelectRegister_i[0].writeRTSI_i_Output_Select(nTriggers::kRTSI_AI_START1);
   masterDevice.Triggers.RTSI_Trig_Direction_Register.writeRTSI0_Pin_Dir(nTriggers::kRTSI_Output, &status);

   // 2. Import the master's trigger on PXI_Trig0
   slaveDevice.Triggers.RTSI_Trig_Direction_Register.writeRTSI0_Pin_Dir(nTriggers::kRTSI_Input, &status);
   // PXI_Trig0 routed to slave's START1 below

   // 3. Export the sample clocks for inspection
   masterDevice.Triggers.PFI_OutputSelectRegister_i[0].writePFI_i_Output_Select(nTriggers::kPFI_AI_Start, &status);
   masterDevice.Triggers.PFI_Direction_Register.writePFI0_Pin_Dir(nTriggers::kPFI_Output, &status);
   slaveDevice.Triggers.PFI_OutputSelectRegister_i[0].writePFI_i_Output_Select(nTriggers::kPFI_AI_Start, &status);
   slaveDevice.Triggers.PFI_Direction_Register.writePFI0_Pin_Dir(nTriggers::kPFI_Output, &status);

   //
   // Program the PLLs
   //

   if (synchronizeDevices)
   {
      // Check bus type
      if (masterDeviceInfo->isPCIe != slaveDeviceInfo->isPCIe)
      {
         printf("Error: Unable to perform trigger skew correction with mixed PCIe and PXIe devices.\n");
         status.setCode(kStatusBadParameter);
         return;
      }

      // Check STC3 Revisions
      if ( masterEepromHelper.getSTC3_Revision() == 1 ||
           slaveEepromHelper.getSTC3_Revision()  == 1    )
      {
         printf("Error: Unable to perform Phase-Lock Loop with STC3 Revision A.\n");
         status.setCode(kStatusBadParameter);
         return;
      }

      if (masterDeviceInfo->isPCIe)
      {
         // Export the master device's 10 MHz reference clock (which is
         // always derived from the on-board 100 MHz oscillator) and Phase-Lock
         // Loop both devices to the exported reference clock. Use RTSI 1
         // since the START1 trigger is on RTSI 0.
         desiredPLL_Source = nTriggers::kRefClkSrc_RTSI1;
         parameters.frequency = nNISTC3::kReference10MHz; // The reference clock is 10 MHz

         masterDevice.Triggers.RTSI_OutputSelectRegister_i[1].writeRTSI_i_Output_Select(nTriggers::kRTSI_RefClkOut);
         masterDevice.Triggers.RTSI_Trig_Direction_Register.writeRTSI1_Pin_Dir(nTriggers::kRTSI_Output, &status);
         slaveDevice.Triggers.RTSI_Trig_Direction_Register.writeRTSI1_Pin_Dir(nTriggers::kRTSI_Input, &status);
      }
      else
      {
         // Phase-Lock Loop both devices to the backplane clock (PXIe_Clk100)
         // Master device
         parameters.frequency = nNISTC3::kReference100MHz; // The backplane clock is 100 MHz
      }

      parameters.pllMultiplier =                      // Multiplier will multiply the refrence frequency...
         static_cast<u16>(100/parameters.frequency);  // ...evenly to 100MHz

      // Master device
      masterPllHelper.enablePLL(parameters, desiredPLL_Source, status);
      if (status.isFatal())
      {
         printf("Error: Master device failed to phase-lock loop.\n");
         return;
      }

      // Slave device
      slavePllHelper.enablePLL(parameters, desiredPLL_Source, status);
      if (status.isFatal())
      {
         printf("Error: Slave device failed to phase-lock loop.\n");
         return;
      }
   }

   /*********************************************************************\
   |
   |   Program the AI subsystem
   |
   \*********************************************************************/

   //
   // Reset the AI subsystems
   //

   masterAiHelper.reset(status);
   slaveAiHelper.reset(status);

   //
   // Program AI timing
   //

   // Enter timing configuration mode
   masterDevice.AI.AI_Timer.Reset_Register.writeConfiguration_Start(kTrue, &status);
   slaveDevice.AI.AI_Timer.Reset_Register.writeConfiguration_Start(kTrue, &status);

   // Master device
   // Disable external gating of the sample clock
   masterAiHelper.programExternalGate(
      nAI::kGate_Disabled,                  // No external pause signal
      nAI::kActive_High_Or_Rising_Edge,     // Don't care
      status);

   // Program the START1 signal (start trigger) to assert from an external signal
   masterAiHelper.programStart1(
      masterTrigger,                        // Set the PFI line
      triggerPolarity,                      // Make line active on...
      kTrue,                                //   ...edge (not level)
      status);

   // Program the START signal (sample clock)
   masterAiHelper.programStart(
      sampClkSrc,
      sampClkPolarity,                      // Make the line active on...
      kTrue,                                //   ...edge (not level)
      status);

   if (masterDeviceInfo->isSimultaneous)
   {
      // Program the convert to be the same as START
      masterAiHelper.programConvert(
         sampClkSrc,
         sampClkPolarity,
         status);
   }
   else
   {
      // Program the convert clock to start on the sample clock
      masterAiHelper.programConvert(
         nAI::kStartCnv_InternalTiming,        // Drive the clock line from internal sample clock
         nAI::kActive_Low_Or_Falling_Edge,     // Convert on falling edge
         status);
   }

   // Program the sample and convert clock timing specifications
   masterTimingConfig.setAcqLevelTimingMode(nNISTC3::kInTimerContinuous, status); // This is a continuous measurement
   masterTimingConfig.setUseSICounter(kTrue, status); // Use SI for internal sample clocking
   masterTimingConfig.setSamplePeriod(samplePeriod, status);
   masterTimingConfig.setSampleDelay(sampleDelay, status);
   masterTimingConfig.setNumberOfSamples(0, status);      // Doesn't matter since this is continuous
   if (!masterDeviceInfo->isSimultaneous)
   {
      masterTimingConfig.setUseSI2Counter(kTrue, status); // Use SI2 for internal convert clocking
      masterTimingConfig.setConvertPeriod(convertPeriod, status);
      masterTimingConfig.setConvertDelay(convertDelay, status);
   }
   if (synchronizeDevices)
   {
      masterTimingConfig.setSyncMode(nInTimer::kSyncMaster, status); // The master device must use master sync
   }

   // Push the timing specification to the device
   masterAiHelper.getInTimerHelper(status).programTiming(masterTimingConfig, status);

   // Program the FIFO width, this method also clears the FIFO
   masterAiHelper.programFIFOWidth(nAI::kTwoByteFifo, status);

   // Slave device
   // Disable external gating of the sample clock
   slaveAiHelper.programExternalGate(
      nAI::kGate_Disabled,                  // No external pause signal
      nAI::kActive_High_Or_Rising_Edge,     // Don't care
      status);

   // Program the START1 signal (start trigger) to assert from the master device
   slaveAiHelper.programStart1(
      slaveTrigger,                         // Set the line
      triggerPolarity,                      // Make line active on...
      kTrue,                                //   ...edge (not level)
      status);

   // Program the START signal (sample clock)
   slaveAiHelper.programStart(
      sampClkSrc,
      sampClkPolarity,                      // Make the line active on...
      kTrue,                                //   ...edge (not level)
      status);

   if (slaveDeviceInfo->isSimultaneous)
   {
      // Program the convert to be the same as START
      slaveAiHelper.programConvert(
         sampClkSrc,
         sampClkPolarity,
         status);
   }
   else
   {
      // Program the convert clock to start on the sample clock
      slaveAiHelper.programConvert(
         nAI::kStartCnv_InternalTiming,        // Drive the clock line from internal sample clock
         nAI::kActive_Low_Or_Falling_Edge,     // Convert on falling edge
         status);
   }

   // Program the sample and convert clock timing specifications
   slaveTimingConfig.setAcqLevelTimingMode(nNISTC3::kInTimerContinuous, status); // This is a continuous measurement
   slaveTimingConfig.setUseSICounter(kTrue, status); // Use SI for internal sample clocking
   slaveTimingConfig.setSamplePeriod(samplePeriod, status);
   slaveTimingConfig.setSampleDelay(sampleDelay, status);
   slaveTimingConfig.setNumberOfSamples(0, status);      // Doesn't matter since this is continuous
   if (!slaveDeviceInfo->isSimultaneous)
   {
      slaveTimingConfig.setUseSI2Counter(kTrue, status); // Use SI2 for internal convert clocking
      slaveTimingConfig.setConvertPeriod(convertPeriod, status);
      slaveTimingConfig.setConvertDelay(convertDelay, status);
   }
   if (synchronizeDevices)
   {
      slaveTimingConfig.setSyncMode(nInTimer::kSyncSlave, status); // The slave device must use slave sync
   }

   // Push the timing specification to the device
   slaveAiHelper.getInTimerHelper(status).programTiming(slaveTimingConfig, status);

   // Program the FIFO width, this method also clears the FIFO
   slaveAiHelper.programFIFOWidth(nAI::kTwoByteFifo, status);

   //
   // Program AI channels
   //

   // Master device
   // Clear configuration FIFO
   masterAiHelper.getInTimerHelper(status).clearConfigurationMemory(status);

   for (u16 i=0; i<numberOfChannels; ++i)
   {
      // Set channel parameters
      masterChanConfig[i].isLastChannel = (i == numberOfChannels-1) ? kTrue : kFalse;  // Last channel?
      masterChanConfig[i].enableDither = nAI::kEnabled;   // Dithering helps increase ADC accuracy
      masterChanConfig[i].gain = masterGain;              // Set the gain (used by the hardware)
      masterChanConfig[i].channelType = termConfig;       // Set the terminal configuration
      masterChanConfig[i].bank = nAI::kBank0;             // AI channels 0..15 are on bank0
      masterChanConfig[i].channel = i%maxChannel;         // aiN where N = i%maxChannel
      masterChanConfig[i].range = range;                  // Set the range (used by the scaler)

      // Push the channel configuration to the device
      masterAiHelper.programChannel(masterChanConfig[i], status);
   }

   // Advance the configuration FIFO
   masterAiHelper.getInTimerHelper(status).primeConfigFifo(status);

   // Slave device
   // Clear configuration FIFO
   slaveAiHelper.getInTimerHelper(status).clearConfigurationMemory(status);

   for (u16 i=0; i<numberOfChannels; ++i)
   {
      // Set channel parameters
      slaveChanConfig[i].isLastChannel = (i == numberOfChannels-1) ? kTrue : kFalse;  // Last channel?
      slaveChanConfig[i].enableDither = nAI::kEnabled;   // Dithering helps increase ADC accuracy
      slaveChanConfig[i].gain = slaveGain;               // Set the gain (used by the hardware)
      slaveChanConfig[i].channelType = termConfig;       // Set the terminal configuration
      slaveChanConfig[i].bank = nAI::kBank0;             // AI channels 0..15 are on bank0
      slaveChanConfig[i].channel = i%maxChannel;         // aiN where N = i%maxChannel
      slaveChanConfig[i].range = range;                  // Set the range (used by the scaler)

      // Push the channel configuration to the device
      slaveAiHelper.programChannel(slaveChanConfig[i], status);
   }

   // Advance the configuration FIFO
   slaveAiHelper.getInTimerHelper(status).primeConfigFifo(status);

   // Leave timing configuration mode
   masterDevice.AI.AI_Timer.Reset_Register.writeConfiguration_End(kTrue, &status);
   slaveDevice.AI.AI_Timer.Reset_Register.writeConfiguration_End(kTrue, &status);

   /*********************************************************************\
   |
   |   Program DMA
   |
   \*********************************************************************/

   std::auto_ptr<nNISTC3::tCHInChDMAChannel> masterDma(new nNISTC3::tCHInChDMAChannel(masterDevice, dmaChannel, status));
   if (status.isFatal())
   {
      printf("Error: Master DMA channel initialization (%d).\n", status);
      return;
   }
   masterDma->reset(status);

   // Use the optimized 2-link SGL ring buffer
   masterDma->configure(bus, nNISTC3::kReuseLinkRing, nNISTC3::kIn, dmaSizeInBytes, status);
   if (status.isFatal())
   {
      printf("Error: Master DMA channel configuration (%d).\n", status);
      return;
   }

   masterDma->start(status);
      if (status.isFatal())
   {
      printf("Error: Master DMA channel start (%d).\n", status);
      return;
   }

   std::auto_ptr<nNISTC3::tCHInChDMAChannel> slaveDma(new nNISTC3::tCHInChDMAChannel(slaveDevice, dmaChannel, status));
   if (status.isFatal())
   {
      printf("Error: Slave DMA channel initialization (%d).\n", status);
      return;
   }
   slaveDma->reset(status);

   // Use the optimized 2-link SGL ring buffer
   slaveDma->configure(bus, nNISTC3::kReuseLinkRing, nNISTC3::kIn, dmaSizeInBytes, status);
   if (status.isFatal())
   {
      printf("Error: Slave DMA channel configuration (%d).\n", status);
      return;
   }

   slaveDma->start(status);
   if (status.isFatal())
   {
      printf("Error: Slave DMA channel start (%d).\n", status);
      return;
   }

   masterStreamHelper.configureForInput(!allowOverwrite, dmaChannel, status);
   slaveStreamHelper.configureForInput(!allowOverwrite, dmaChannel, status);
   if (!allowOverwrite)
   {
      masterStreamHelper.modifyTransferSize(dmaSizeInBytes, status);
      slaveStreamHelper.modifyTransferSize(dmaSizeInBytes, status);
   }
   masterStreamHelper.enable(status);
   slaveStreamHelper.enable(status);

   /*********************************************************************\
   |
   |   Start the voltage measurement
   |
   \*********************************************************************/

   //
   // Arm the AI subsystems
   //

   masterAiHelper.getInTimerHelper(status).armTiming(masterTimingConfig, status);
   slaveAiHelper.getInTimerHelper(status).armTiming(slaveTimingConfig, status);

   //
   // Wait for the start trigger
   //

   elapsedTime = 0;
   start = clock();
   while (masterDevice.AI.AI_Timer.Status_2_Register.readSC_Q_St(&status) != nInTimer::kSC_Counting)
   {
      printf("Waiting %.2f seconds for start trigger...\r", triggerWaitTime-elapsedTime);
      if (elapsedTime > triggerWaitTime)
      {
         printf("\n");
         printf("Error: Trigger timeout.\n");
         status.setCode(kStatusRLPTimeout);
         return;
      }

      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
   }
   printf("\n");

   /*********************************************************************\
   |
   |   Read and print data
   |
   \*********************************************************************/

   nNISTC3::nAIDataHelper::printHeader(numberOfDevices, numberOfChannels, printVolts ? "Voltage" : "ADC code");

   elapsedTime = 0;
   start = clock();
   while (elapsedTime < runTime)
   {
      // 1. Use the read() method to query the number of bytes in the DMA buffers
      masterDma->read(0, NULL, &masterBytesAvailable, allowOverwrite, &dataOverwritten, status);
      slaveDma->read(0, NULL, &slaveBytesAvailable, allowOverwrite, &dataOverwritten, status);
      if (status.isFatal())
      {
         dmaErr = status;
         dmaErrored = kTrue;
      }

      // 2. If there is enough data in the buffers, read, scale, and print it
      else if ( masterBytesAvailable >= readSizeInBytes &&
                slaveBytesAvailable  >= readSizeInBytes    )
      {
         masterDma->read(readSizeInBytes, reinterpret_cast<u8 *>(&masterRawData[0]), &masterBytesAvailable, allowOverwrite, &dataOverwritten, status);
         slaveDma->read(readSizeInBytes, reinterpret_cast<u8 *>(&slaveRawData[0]), &slaveBytesAvailable, allowOverwrite, &dataOverwritten, status);
         if (status.isNotFatal())
         {
            if (printVolts)
            {
               // Scale
               nNISTC3::nAIDataHelper::scaleData(masterRawData, totalNumberOfSamples,
                                                 masterScaledData, totalNumberOfSamples,
                                                 masterChanConfig, masterEepromHelper, *masterDeviceInfo);
               nNISTC3::nAIDataHelper::scaleData(slaveRawData, totalNumberOfSamples,
                                                 slaveScaledData, totalNumberOfSamples,
                                                 slaveChanConfig, slaveEepromHelper, *slaveDeviceInfo);

               // Interleave
               nNISTC3::nAIDataHelper::interleaveData(totalNumberOfSamples, numberOfChannels,
                                                     jointScaledData, masterScaledData, slaveScaledData);

               // Print
               nNISTC3::nAIDataHelper::printData(jointScaledData,
                                                 totalNumberOfSamples*numberOfDevices,
                                                 numberOfChannels*numberOfDevices);
            }
            else
            {
               // Interleave
               nNISTC3::nAIDataHelper::interleaveData(totalNumberOfSamples, numberOfChannels,
                                                      jointRawData, masterRawData, slaveRawData);

               // Print
               nNISTC3::nAIDataHelper::printData(jointRawData,
                                                 totalNumberOfSamples*numberOfDevices,
                                                 numberOfChannels*numberOfDevices);
            }
            bytesRead += readSizeInBytes;

            // Permit the Stream Circuits to transfer another readSizeInBytes bytes
            if (!allowOverwrite)
            {
               masterStreamHelper.modifyTransferSize(readSizeInBytes, status);
               slaveStreamHelper.modifyTransferSize(readSizeInBytes, status);
            }
         }
         else
         {
            dmaErr = status;
            dmaErrored = kTrue;
         }
      }

      // 3. Check for AI subsystem errors
      masterDevice.AI.AI_Timer.Status_1_Register.refresh(&status);
      masterScanOverrun = masterDevice.AI.AI_Timer.Status_1_Register.getScanOverrun_St(&status);
      masterAdcOverrun = masterDevice.AI.AI_Timer.Status_1_Register.getOverrun_St(&status);
      if (masterDeviceInfo->isSimultaneous)
      {
         masterFifoOverflow = static_cast<nInTimer::tInTimer_Error_t>(masterDevice.SimultaneousControl.InterruptStatus.readAiFifoOverflowInterruptCondition(&status));
      }
      else
      {
         masterFifoOverflow = masterDevice.AI.AI_Timer.Status_1_Register.getOverflow_St(&status);
      }

      slaveDevice.AI.AI_Timer.Status_1_Register.refresh(&status);
      slaveScanOverrun = slaveDevice.AI.AI_Timer.Status_1_Register.getScanOverrun_St(&status);
      slaveAdcOverrun = slaveDevice.AI.AI_Timer.Status_1_Register.getOverrun_St(&status);
      if (slaveDeviceInfo->isSimultaneous)
      {
         slaveFifoOverflow = static_cast<nInTimer::tInTimer_Error_t>(slaveDevice.SimultaneousControl.InterruptStatus.readAiFifoOverflowInterruptCondition(&status));
      }
      else
      {
         slaveFifoOverflow = slaveDevice.AI.AI_Timer.Status_1_Register.getOverflow_St(&status);
      }

      if (masterScanOverrun || masterAdcOverrun || masterFifoOverflow)
      {
         masterAiErrored = kTrue;
      }
      if (slaveScanOverrun || slaveAdcOverrun || slaveFifoOverflow)
      {
         slaveAiErrored = kTrue;
      }

      if (masterAiErrored || slaveAiErrored || dmaErrored)
      {
         break;
      }

      // 4. Update the run time for this measurement
      elapsedTime = static_cast<f64>(clock() - start) / CLOCKS_PER_SEC;
   }

   /*********************************************************************\
   |
   |   Stop the voltage measurement
   |
   \*********************************************************************/

   if (!masterAiErrored)
   {
      // Stop and disarm the AI timing engine
      masterDevice.AI.AI_Timer.Command_Register.writeEnd_On_End_Of_Scan(kTrue, &status);

      // Wait for the last scan to complete
      rlpElapsedTime = 0;
      rlpStart = clock();
      while (masterDevice.AI.AI_Timer.Status_1_Register.readSC_Armed_St(&status))
      {
         // Spin on the SC Armed bit
         if (rlpElapsedTime > rlpTimeout)
         {
            printf("\n");
            printf("Error: Master AI timing engine did not stop within timeout.\n");
            status.setCode(kStatusRLPTimeout);
            return;
         }
         rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
      }

      // Wait for the Stream Circuit to empty its FIFO
      rlpElapsedTime = 0;
      rlpStart = clock();
      while (!masterStreamHelper.fifoIsEmpty(status))
      {
         // Spin on the FifoEmpty bit
         if (rlpElapsedTime > rlpTimeout)
         {
            printf("\n");
            printf("Error: Master stream circuit did not flush within timeout.\n");
            status.setCode(kStatusRLPTimeout);
            return;
         }
         rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
      }
   }
   if (!slaveAiErrored)
   {
      // Stop and disarm the AI timing engine
      slaveDevice.AI.AI_Timer.Command_Register.writeEnd_On_End_Of_Scan(kTrue, &status);

      // Wait for the last scan to complete
      rlpElapsedTime = 0;
      rlpStart = clock();
      while (slaveDevice.AI.AI_Timer.Status_1_Register.readSC_Armed_St(&status))
      {
         // Spin on the SC Armed bit
         if (rlpElapsedTime > rlpTimeout)
         {
            printf("\n");
            printf("Error: Slave AI timing engine did not stop within timeout.\n");
            status.setCode(kStatusRLPTimeout);
            return;
         }
         rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
      }

      // Wait for the Stream Circuit to empty its FIFO
      rlpElapsedTime = 0;
      rlpStart = clock();
      while (!slaveStreamHelper.fifoIsEmpty(status))
      {
         // Spin on the FifoEmpty bit
         if (rlpElapsedTime > rlpTimeout)
         {
            printf("\n");
            printf("Error: Slave stream circuit did not flush within timeout.\n");
            status.setCode(kStatusRLPTimeout);
            return;
         }
         rlpElapsedTime = static_cast<f64>(clock() - rlpStart) / CLOCKS_PER_SEC;
      }
   }

   /*********************************************************************\
   |
   |   Flush the DMA buffer
   |
   \*********************************************************************/

   //
   // Disable DMA on the devices and stop the CHInCh DMA channels
   //

   slaveStreamHelper.disable(status);
   masterStreamHelper.disable(status);

   slaveDma->stop(status);
   masterDma->stop(status);

   //
   // Read remaining samples from the DMA buffers
   //

   masterDma->read(0, NULL, &masterBytesAvailable, allowOverwrite, &dataOverwritten, status);
   slaveDma->read(0, NULL, &slaveBytesAvailable, allowOverwrite, &dataOverwritten, status);
   if (status.isFatal())
   {
      dmaErr = status;
      dmaErrored = kTrue;
   }
   while (masterBytesAvailable && slaveBytesAvailable)
   {
      if (masterBytesAvailable < readSizeInBytes)
      {
         readSizeInBytes = masterBytesAvailable;
      }
      masterDma->read(readSizeInBytes, reinterpret_cast<u8 *>(&masterRawData[0]), &masterBytesAvailable, allowOverwrite, &dataOverwritten, status);
      slaveDma->read(readSizeInBytes, reinterpret_cast<u8 *>(&slaveRawData[0]), &slaveBytesAvailable, allowOverwrite, &dataOverwritten, status);
      if (status.isNotFatal())
      {
         if (printVolts)
         {
            // Scale
            nNISTC3::nAIDataHelper::scaleData(masterRawData, readSizeInBytes/sampleSizeInBytes,
                                              masterScaledData, readSizeInBytes/sampleSizeInBytes,
                                              masterChanConfig, masterEepromHelper, *masterDeviceInfo);
            nNISTC3::nAIDataHelper::scaleData(slaveRawData, readSizeInBytes/sampleSizeInBytes,
                                              slaveScaledData, readSizeInBytes/sampleSizeInBytes,
                                              slaveChanConfig, slaveEepromHelper, *slaveDeviceInfo);

            // Interleave
            nNISTC3::nAIDataHelper::interleaveData(readSizeInBytes/sampleSizeInBytes, numberOfChannels,
                                                   jointScaledData, masterScaledData, slaveScaledData);

            // Print
            nNISTC3::nAIDataHelper::printData(jointScaledData,
                                              readSizeInBytes/sampleSizeInBytes*numberOfDevices,
                                              numberOfChannels*numberOfDevices);
         }
         else
         {
            // Interleave
            nNISTC3::nAIDataHelper::interleaveData(readSizeInBytes/sampleSizeInBytes, numberOfChannels,
                                                   jointRawData, masterRawData, slaveRawData);

            // Print
            nNISTC3::nAIDataHelper::printData(jointRawData,
                                              readSizeInBytes/sampleSizeInBytes*numberOfDevices,
                                              numberOfChannels*numberOfDevices );
         }
         bytesRead += readSizeInBytes;
      }
      else
      {
         dmaErr = status;
         dmaErrored = kTrue;
         break;
      }
   }
   printf("\n");

   /*********************************************************************\
   |
   |   Finalize the measurement
   |
   \*********************************************************************/

   //
   // Print run-time summary
   //

   if (dmaErrored)
   {
      printf("Error: DMA read (%d).\n", dmaErr);
      status.setCode(kStatusRuntimeError);
   }
   if (masterFifoOverflow)
   {
      printf("Error: Master AI FIFO overflow.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (masterScanOverrun)
   {
      printf("Error: Master AI sample clock overrun.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (masterAdcOverrun)
   {
      printf("Error: Master AI ADC overrun.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (slaveFifoOverflow)
   {
      printf("Error: Slave AI FIFO overflow.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (slaveScanOverrun)
   {
      printf("Error: Slave AI sample clock overrun.\n");
      status.setCode(kStatusRuntimeError);
   }
   if (slaveAdcOverrun)
   {
      printf("Error: Slave AI ADC overrun.\n");
      status.setCode(kStatusRuntimeError);
   }

   if (! (masterAiErrored || slaveAiErrored || dmaErrored))
   {
      printf("Finished continuous %.2f-second hardware-timed analog measurement.\n", runTime);
      printf("Read %u samples from each device (%s) using a %u-sample DMA buffer.\n",
             bytesRead/sampleSizeInBytes,
             dataOverwritten ? "by overwriting data" : "without overwriting data",
             dmaSizeInBytes/sampleSizeInBytes);
   }

   //
   // Restore the state of the devices
   //

   // Disable the PLLs
   if (synchronizeDevices)
   {
      slavePllHelper.disablePLL(status);
      masterPllHelper.disablePLL(status);
   }

   // Nothing else to do: the helpers' destructors safely unwind device state.
}
