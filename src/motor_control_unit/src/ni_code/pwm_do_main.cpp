#include <memory>

#include <DigitalInputOutputTask.h>

nMDBG::tStatus2 status;

std::unique_ptr<tXSeries> GetDevice(iBus *bus)
{
  if (bus == NULL)
  {
    return nullptr;
  }

  auto bar0 = bus->createAddressSpace(kPCI_BAR0);
  auto device = std::make_unique<tXSeries>(bar0, &status);

  return std::move(device);
}

std::unique_ptr<std::vector<unsigned char>> GenerateWaveform(
  const unsigned int invertMaskPort0, const unsigned int samplesPerChannel,
  const unsigned int sampleSizeInBytes)
{
  auto data = std::make_unique<std::vector<unsigned char>>(
    samplesPerChannel * sampleSizeInBytes);
  unsigned char* dataOneByte = &(data->at(0));
  unsigned int* dataFourBytes = reinterpret_cast<unsigned int*>(&(data->at(0)));
  for (auto j{0u}; j < samplesPerChannel; ++j)
  {
    if (sampleSizeInBytes == 1)
    {
      dataOneByte[j] = static_cast<unsigned char>(j ^ invertMaskPort0);
    }
    else
    {
      dataFourBytes[j] = j ^ invertMaskPort0;
    }
  }
  return std::move(data);
}

int main(int argc, char *argv[])
{
  if (argc <= 2)
  {
    printf("Usage: pwm_do_main.cpp [bus number] [device number]\n");
    exit(-1);
  }

  /* tXSeries */
  // get tXSeries device object
  auto busNumber = argv[1];
  auto deviceNumber = argv[2];
  char boardLocation[256];
  sprintf(boardLocation, "PXI%s::%s::INSTR", busNumber, deviceNumber);
  iBus *bus = acquireBoard(boardLocation);

  std::unique_ptr<tXSeries> device = GetDevice(bus);
  if (device == nullptr)
  {
    printf("tXSeries device is nullptr\n");
    return -1;
  }

  auto deviceInfo = nNISTC3::getDeviceInfo(*device, status);
  auto port0Length = deviceInfo->port0Length;

  if (deviceInfo->isSimultaneous)
  {
    printf("Device is simultaneous\n");
    nNISTC3::initializeSimultaneousXSeries(*device, status);
  }

  /* dioHelper */
  // configures DIO lines with dioHelper
  // dioHelper has
  //   - tDI for controlling digital input
  //   - tDO for controlling digital output
  auto dioHelper = std::make_unique<nNISTC3::dioHelper>(
    device->DI, device->DO, status);

  // reset the device to make sure it's in default state for configuration
  // use the same arguments as in the dtor of dioHelper
  dioHelper->reset(NULL, 0, status);

  // set tristate so it will automatically reset() when calling dtor
  dioHelper->setTristate(kTrue, status);


  /* program circuitry in DO */
  //   - START1 trigger
  //   - Update Interval (UI)
  //   - External Gate (for whichever counter it needs)
  //   - Update Counter (CI)
  //   - Buffer Counter (BC)

  // holds the circuit in reset to prevent glitches with OutTimer's Reset_Register
  device->DO.DO_Timer.Reset_Register.writeConfiguration_Start(kTrue, &status);

  /* configure DO output Channel */
  // actually don't understand this, but 0 = port0?
  // or 0u is the result of 1u - 1 = 0u? so it actually means number of channel = 1?
  device->DO.DO_Timer.Output_Control_Register.setNumber_Of_Channels(0u, &status);

  /* Configure DO output lines */
  // using dioHelper class object for this
  // tDigDataType = u32 = unsigned int
  dioHelper->configureLines(0x00FFFFFF, nNISTC3::kCorrOutput, status);

  /* START1 Trigger */
  device->DO.DO_Trigger_Select_Register.setDO_START1_Select(nDO::kStart1_Pulse, &status);
  device->DO.DO_Trigger_Select_Register.setDO_START1_Polarity(nDO::kRising_Edge, &status);
  device->DO.DO_Trigger_Select_Register.setDO_START1_Edge(kTrue, &status);

  // configure START1 Trigger to not use external gate
  device->DO.DO_Trigger_Select_Register.setDO_External_Gate_Enable(kFalse, &status);
  device->DO.DO_Trigger_Select_Register.flush(&status);

  /* Update Interval */
  // Update interval, aka tOutTimer within tDO, aka sample clock
  device->DO.DO_Timer.Mode_2_Register.setSyncMode(nOutTimer::kSyncDefault, &status);
  device->DO.DO_Timer.Mode_2_Register.setStart1_Export_Mode(
    nOutTimer::kExportSynchronizedTriggers, &status);
  device->DO.DO_Timer.Mode_2_Register.flush(&status);

  device->DO.DO_Timer.Mode_1_Register.setUI_Source_Select(nOutTimer::kUI_Src_TB3, &status);
  device->DO.DO_Timer.Mode_1_Register.setUI_Source_Polarity(nOutTimer::kRising_Edge, &status);

  // set Update interval reload to no change for continuous sample mode
  device->DO.DO_Timer.Mode_1_Register.setUI_Reload_Mode(nOutTimer::kUI_Reload_No_Change, &status);

  device->DO.DO_Timer.Mode_1_Register.flush(&status);

  // load UI from register
  auto sampleDelay = 2u;
  auto samplePeriod = 100000u; // rate: 100 MHz / 100e3 = 1 kHz
  device->DO.DO_Timer.Mode_1_Register.writeUI_Initial_Load_Source(nOutTimer::kReg_A, &status);
  device->DO.DO_Timer.UI_Load_A_Register.writeRegister(sampleDelay - 1, &status);
  device->DO.DO_Timer.Command_1_Register.writeUI_Load(1u, &status);
  device->DO.DO_Timer.UI_Load_B_Register.writeRegister(samplePeriod - 1, &status);
  device->DO.DO_Timer.Mode_1_Register.writeUI_Initial_Load_Source(nOutTimer::kReg_B, &status);

  // configure DO to activate START1 trigger when Update Interval Termination Count condition is met
  // UI_TC = update interval termination count
  device->DO.DO_Trigger_Select_Register.setDO_UPDATE_Source_Select(nDO::kUpdate_UI_TC, &status);
  device->DO.DO_Trigger_Select_Register.setDO_UPDATE_Source_Polarity(nDO::kRising_Edge, &status);
  device->DO.DO_Trigger_Select_Register.flush(&status);

  // configure the stop conditions
  device->DO.DO_Timer.Mode_1_Register.setContinuous(nOutTimer::kContinuousOp, &status);
  device->DO.DO_Timer.Mode_1_Register.setTrigger_Once(kTrue, &status);
  device->DO.DO_Timer.Mode_1_Register.flush(&status);

  device->DO.DO_Timer.Mode_2_Register.setStop_On_Overrun_Error(nOutTimer::kStop_on_Error, &status);
  device->DO.DO_Timer.Mode_2_Register.setStop_On_BC_TC_Trigger_Error(
    nOutTimer::kContinue_on_Error, &status);
  device->DO.DO_Timer.Mode_2_Register.setStop_On_BC_TC_Error(nOutTimer::kContinue_on_Error, &status);
  device->DO.DO_Timer.Mode_2_Register.flush(&status);

  /* FIFO */
  // chooses the data FIFO condition on which to generate the FIFO interrupt
  device->DO.DO_Timer.Mode_1_Register.setFIFO_Mode(nOutTimer::kFifoMode_Less_Than_Full, &status);

  // enable local buffer mode, aka fifo buffer regeneration if onboard memory supports it
  // interacts with DOFFRT signal
  device->DO.DO_Timer.Mode_1_Register.setFIFO_Retransmit_Enable(nOutTimer::kEnabled, &status);
  device->DO.DO_Timer.Mode_1_Register.flush(&status);

  // enable FIFO
  device->DO.DO_Timer.Mode_2_Register.setFIFO_Enable(nDO::kEnabled, &status);
  device->DO.DO_Timer.Mode_2_Register.flush(&status);

  // configure FIFO memory size
  auto dataWidth = (port0Length == 32) ? nDO::kDO_FourBytes : nDO::kDO_OneByte;
  auto dataLane = nDO::kDO_DataLane0;
  device->DO.DO_Mode_Register.setDO_DataWidth(dataWidth, &status);
  device->DO.DO_Mode_Register.setDO_Data_Lane(dataLane, &status);
  device->DO.DO_Mode_Register.flush(&status);

  // reset/clear FIFO before start
  device->DO.DO_Timer.Reset_Register.writeFIFO_Clear(1u, &status);

  /* Update Counter */
  // each buffer should contain a complete incrementing waveform (?)
  auto samplesPerChannel = 256u;
  device->DO.DO_Timer.UC_Load_A_Register.writeRegister(samplesPerChannel - 1u, &status);
  device->DO.DO_Timer.UC_Load_B_Register.writeRegister(samplesPerChannel - 1u, &status);
  device->DO.DO_Timer.Mode_1_Register.writeUC_Reload_Mode(
    nOutTimer::kAlternate_First_Period_Every_BC_TC, &status);

  // Load Update Counter
  // Why does it not specify any value for Register A?
  // Why does it later choose register B as load source?
  device->DO.DO_Timer.Mode_1_Register.writeUC_Initial_Load_Source(nOutTimer::kReg_A, &status);
  device->DO.DO_Timer.Command_1_Register.writeUC_Load(1u, &status);
  device->DO.DO_Timer.Mode_1_Register.writeUC_Initial_Load_Source(nOutTimer::kReg_B, &status);

  /* Buffer Counter */
  device->DO.DO_Timer.Mode_1_Register.setBC_Initial_Load_Source(nOutTimer::kReg_A, &status);
  device->DO.DO_Timer.Mode_1_Register.setBC_Reload_Mode(nOutTimer::kBC_Reload_No_Change, &status);
  device->DO.DO_Timer.Mode_1_Register.flush(&status);

  // load value from register A into buffer counter
  device->DO.DO_Timer.BC_Load_A_Register.writeRegister(1u, &status);
  device->DO.DO_Timer.Command_1_Register.writeBC_Load(1u, &status);

  // disable gates for buffer counter
  device->DO.DO_Timer.Mode_2_Register.writeBC_Gate_Enable(nOutTimer::kEnabled, &status);
  // device->DO.DO_Timer.Mode_2_Register.writeBC_Gate_Enable(nOutTimer::kDisabled, &status);


  /* finish configuration */
  // when Configuration_End is set to 1 (true), it will set Configuration_Start to 0
  device->DO.DO_Timer.Reset_Register.writeConfiguration_End(kTrue, &status);


  /* program DMA */

  // generate output waveform and store in rawData
  auto sampleSizeInBytes = (port0Length == 32) ? 4u : 1u;
  unsigned int invertMaskPort0 = 0x0;
  invertMaskPort0 &= (port0Length == 32)
    ? static_cast<unsigned int>(nDI::nDI_FIFO_Data_Register::nCDI_FIFO_Data::kMask)
    : static_cast<unsigned int>(nDI::nDI_FIFO_Data_Register8::nCDI_FIFO_Data8::kMask);
  std::unique_ptr<std::vector<unsigned char>> waveform =
    GenerateWaveform(invertMaskPort0, samplesPerChannel, sampleSizeInBytes);

  nNISTC3::nDIODataHelper::printHeader(0);
  nNISTC3::nDIODataHelper::printData(
    *waveform, samplesPerChannel * sampleSizeInBytes, sampleSizeInBytes);

  // start pulse
  // cleanup


  return 0;
}
