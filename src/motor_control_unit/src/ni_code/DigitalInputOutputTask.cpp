#include <DigitalInputOutputTask.h>

DigitalInputOutputTask::DigitalInputOutputTask()
{}

int DigitalInputOutputTask::Init(const char *busNumber, const char *deviceNumber)
{
  sprintf(boardLocation, "PXI%s::%s::INSTR", busNumber, deviceNumber);
  bus = acquireBoard(boardLocation);
  if (bus == NULL)
  {
    printf("Could not access PCI device. Exiting.\n");
    return -1;
  }

  lineMaskPort0 = 0xFFFFFFFF; //use all lines on port 0
  lineMaskPort1 = 0xFF; //use all lines on port 1
  lineMaskPort2 = 0xFF; //use all lines on port 2
  triStateOnExit = kTrue;

  outputDataPort0 = 0x5A5A5A5A; // value to write to port0
  outputDataPort1 = 0x99; // value to write to port1
  outputDataPort2 = 0x66; // value to write to port2

  port0Length = 0;
  port1Length = 8;

  lineMaskPFI = (lineMaskPort2 << port1Length) | lineMaskPort1;
  outputDataPFI = (outputDataPort2 << port1Length) | outputDataPort1;
  inputDataPort0 = 0x0; // value of lines on port 0
  inputDataPFI = 0x0; //value of lines on port1:2 (PFI0..15)

  bar0 = bus->createAddressSpace(kPCI_BAR0);
  device = std::make_unique<tXSeries>(bar0, &status);

  const nNISTC3::tDeviceInfo *deviceInfo = nNISTC3::getDeviceInfo(*device, status);
  if (status.isFatal())
  {
    printf("Error: cannot identify device (%d)\n", status.statusCode);
    return -1;
  }

  if (deviceInfo->isSimultaneous)
  {
    nNISTC3::initializeSimultaneousXSeries(*device, status);
  }

  port0Length = deviceInfo->port0Length;
  if (port0Length == 32)
  {
    // port 0 has 32 lines for hardware-timed DIO
    lineMaskPort0 &=
      static_cast<unsigned int>(nDI::nDI_FIFO_Data_Register::nCDI_FIFO_Data::kMask);
  }
  else
  {
    // port 0 has 8 lines for hardware-timed DIO
    lineMaskPort0 &=
      static_cast<unsigned int>(nDI::nDI_FIFO_Data_Register8::nCDI_FIFO_Data8::kMask);
  }

  dioHelper = std::make_unique<nNISTC3::dioHelper>(device->DI, device->DO, status);
  dioHelper->setTristate(triStateOnExit, status);
  pfiDioHelper = std::make_unique<nNISTC3::pfiDioHelper>(device->Triggers, status);
  pfiDioHelper->setTristate(triStateOnExit, status);

  dioHelper->reset(NULL, 0, status);
  dioHelper->configureLines(lineMaskPort0, nNISTC3::kOutput, status);
  pfiDioHelper->reset(NULL, 0, status);
  pfiDioHelper->configureLines(lineMaskPFI, nNISTC3::kOutput, status);
}

int DigitalInputOutputTask::Write()
{
  printf("Writing 0x%0X to port0.\n", outputDataPort0 & lineMaskPort0);
  dioHelper->writePresentValue(lineMaskPort0, outputDataPort0, status);

  printf("Writing 0x%0X to port1.\n", outputDataPort1 & lineMaskPort1);
  printf("Writing 0x%0X to port2.\n", outputDataPort2 & lineMaskPort2);
  pfiDioHelper->writePresentValue(lineMaskPFI, outputDataPFI, status);
}

DigitalInputOutputTask::~DigitalInputOutputTask()
{
  dioHelper.reset();
  pfiDioHelper.reset();
  releaseBoard(bus);
}
