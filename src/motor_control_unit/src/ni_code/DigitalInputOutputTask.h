#ifndef _DIGITALINPUTOUTPUTTASK_H_
#define _DIGITALINPUTOUTPUTTASK_H_

#include <stdio.h>

#include <memory>

#include <dataHelper.h>
#include <osiBus.h>
#include <osiTypes.h>
#include <simultaneousInit.h>
#include <tXSeries.h>
#include <dio/dioHelper.h>
#include <dio/pfiDioHelper.h>

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
  DigitalInputOutputTask();
  int Init(const char *busNumber, const char *deviceNumber);
  int Write();
  ~DigitalInputOutputTask();
};

#endif // _DIGITALINPUTOUTPUTTASK_H_
