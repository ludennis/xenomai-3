#include <stdio.h>

#include <memory>

#include <dataHelper.h>
#include <osiBus.h>
#include <osiTypes.h>
#include <simultaneousInit.h>
#include <tXSeries.h>
#include <dio/dioHelper.h>
#include <dio/pfiDioHelper.h>

// TODO: make this a RT_xenomai periodic Task
class DigitalInputOutputTask
{
private:
  iBus *bus;
  nMDBG::tStatus2 status;
  tAddressSpace bar0;
  std::unique_ptr<tXSeries> device;
  std::unique_ptr<nNISTC3::dioHelper> dioHelper;
  std::unique_ptr<nNISTC3::pfiDioHelper> pfiDioHelper;

public:
  char boardLocation[256];

  unsigned int lineMaskPort0; //use all lines on port 0
  unsigned char lineMaskPort1; //use all lines on port 1
  unsigned char lineMaskPort2; //use all lines on port 2
  unsigned int triStateOnExit;

  unsigned int outputDataPort0; // value to write to port0
  unsigned int outputDataPort1; // value to write to port1
  unsigned int outputDataPort2; // value to write to port2

  unsigned int port0Length;
  unsigned int port1Length;
  unsigned int lineMaskPFI;

  unsigned int outputDataPFI;
  unsigned int inputDataPort0; // value of lines on port 0
  unsigned int inputDataPFI; //value of lines on port1:2 (PFI0..15)

public:
  DigitalInputOutputTask(){}

  int init(const char *busNumber, const char *deviceNumber)
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

  int Write()
  {
    printf("Writing 0x%0X to port0.\n", outputDataPort0 & lineMaskPort0);
    dioHelper->writePresentValue(lineMaskPort0, outputDataPort0, status);

    printf("Writing 0x%0X to port1.\n", outputDataPort1 & lineMaskPort1);
    printf("Writing 0x%0X to port2.\n", outputDataPort2 & lineMaskPort2);
    pfiDioHelper->writePresentValue(lineMaskPFI, outputDataPFI, status);
  }

  ~DigitalInputOutputTask()
  {
    dioHelper.reset();
    pfiDioHelper.reset();
    releaseBoard(bus);
  }
};

int main(int argc, char* argv[])
{
  if (argc <= 2)
  {
    printf("Usage: do_main [PCI Bus Number] [PCI Device Number]\n");
    return -1;
  }

  DigitalInputOutputTask dioTask;
  dioTask.init(argv[1], argv[2]);
  dioTask.Write();

  return 0;
}
