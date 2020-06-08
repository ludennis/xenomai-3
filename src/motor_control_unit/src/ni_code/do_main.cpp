#include <stdio.h>

#include <memory>

#include <dataHelper.h>
#include <osiBus.h>
#include <osiTypes.h>
#include <simultaneousInit.h>
#include <tXSeries.h>
#include <dio/dioHelper.h>
#include <dio/pfiDioHelper.h>

int main(int argc, char* argv[])
{
  if (argc <= 2)
  {
    printf("Usage: do_main [PCI Bus Number] [PCI Device Number]\n");
    return -1;
  }

  // obtain bus
  char boardLocation[256];
  sprintf(boardLocation, "PXI%s::%s::INSTR", argv[1], argv[2]);

  iBus *bus = NULL;
  bus = acquireBoard(boardLocation);

  if (bus == NULL)
  {
    printf("Could not access PCI device. Exiting.\n");
    return -1;
  }

  // initialize parameters
  unsigned int lineMaskPort0 = 0xFFFFFFFF; //use all lines on port 0
  const unsigned char lineMaskPort1 = 0xFF; //use all lines on port 1
  const unsigned char lineMaskPort2 = 0xFF; //use all lines on port 2
  const unsigned int triStateOnExit = kTrue;

  const unsigned int outputDataPort0 = 0x5A5A5A5A; // value to write to port0
  const unsigned int outputDataPort1 = 0x99; // value to write to port1
  const unsigned int outputDataPort2 = 0x66; // value to write to port2

  unsigned int port0Length = 0;
  const unsigned int port1Length = 8;
  const unsigned int lineMaskPFI = (lineMaskPort2 << port1Length) | lineMaskPort1;

  const unsigned int outputDataPFI = (outputDataPort2 << port1Length) | outputDataPort1;
  unsigned int inputDataPort0 = 0x0; // value of lines on port 0
  unsigned int inputDataPFI = 0x0; //value of lines on port1:2 (PFI0..15)

  nMDBG::tStatus2 status;
  tAddressSpace bar0;
  bar0 = bus->createAddressSpace(kPCI_BAR0);

  // access x series device
  tXSeries device(bar0, &status);
  const nNISTC3::tDeviceInfo *deviceInfo = nNISTC3::getDeviceInfo(device, status);
  if (status.isFatal())
  {
    printf("Error: cannot identify device (%d)\n", status.statusCode);
    return -1;
  }

  if (deviceInfo->isSimultaneous)
  {
    nNISTC3::initializeSimultaneousXSeries(device, status);
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

  // subsystem
  auto dioHelper = std::make_unique<nNISTC3::dioHelper>(device.DI, device.DO, status);
  dioHelper->setTristate(triStateOnExit, status);
  auto pfiDioHelper = std::make_unique<nNISTC3::pfiDioHelper>(device.Triggers, status);
  pfiDioHelper->setTristate(triStateOnExit, status);

  dioHelper->reset(NULL, 0, status);
  dioHelper->configureLines(lineMaskPort0, nNISTC3::kOutput, status);
  pfiDioHelper->reset(NULL, 0, status);
  pfiDioHelper->configureLines(lineMaskPFI, nNISTC3::kOutput, status);

  // write data
  printf("Writing 0x%0X to port0.\n", outputDataPort0 & lineMaskPort0);
  dioHelper->writePresentValue(lineMaskPort0, outputDataPort0, status);

  printf("Writing 0x%0X to port1.\n", outputDataPort1 & lineMaskPort1);
  printf("Writing 0x%0X to port2.\n", outputDataPort2 & lineMaskPort2);
  pfiDioHelper->writePresentValue(lineMaskPFI, outputDataPFI, status);

  // delete resources used to access the bus before releasing
  dioHelper.reset();
  pfiDioHelper.reset();

  // release board from bus
  releaseBoard(bus);

  return 0;
}
