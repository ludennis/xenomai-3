#include <DigitalInputOutputTask.h>

nMDBG::tStatus2 DigitalInputOutputTask::status;
std::unique_ptr<nNISTC3::dioHelper> DigitalInputOutputTask::dioHelper;
std::unique_ptr<nNISTC3::pfiDioHelper> DigitalInputOutputTask::pfiDioHelper;
unsigned int DigitalInputOutputTask::lineMaskPort0;
unsigned char DigitalInputOutputTask::lineMaskPort1;
unsigned char DigitalInputOutputTask::lineMaskPort2;
unsigned int DigitalInputOutputTask::lineMaskPFI;
unsigned int DigitalInputOutputTask::outputDataPort0;
unsigned int DigitalInputOutputTask::outputDataPort1;
unsigned int DigitalInputOutputTask::outputDataPort2;
unsigned int DigitalInputOutputTask::outputDataPFI;

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
