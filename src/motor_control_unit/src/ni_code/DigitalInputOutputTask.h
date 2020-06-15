#ifndef _DIGITALINPUTOUTPUTTASK_H_
#define _DIGITALINPUTOUTPUTTASK_H_

#include <stdio.h>

#include <memory>

#include <dataHelper.h>
#include <osiBus.h>
#include <osiTypes.h>
#include <simultaneousInit.h>
#include <tXSeries.h>
#include <inTimer/diHelper.h>
#include <dio/dioHelper.h>
#include <dio/pfiDioHelper.h>

class DigitalInputOutputTask
{
protected:
  iBus *bus;
  static nMDBG::tStatus2 status;
  tAddressSpace bar0;
  std::unique_ptr<tXSeries> device;
  static std::unique_ptr<nNISTC3::diHelper> diHelper;
  static std::unique_ptr<nNISTC3::dioHelper> dioHelper;
  static std::unique_ptr<nNISTC3::pfiDioHelper> pfiDioHelper;

public:
  char boardLocation[256];

  static unsigned int lineMaskPort0;
  static unsigned char lineMaskPort1;
  static unsigned char lineMaskPort2;
  unsigned int triStateOnExit;

  static unsigned int outputDataPort0;
  static unsigned int outputDataPort1;
  static unsigned int outputDataPort2;

  static unsigned int port0Length;
  static unsigned int port1Length;
  static unsigned int lineMaskPFI;

  static unsigned int outputDataPFI;
  static unsigned int inputDataPort0;
  static unsigned int inputDataPFI;

public:
  DigitalInputOutputTask();
  int Init(const char *busNumber, const char *deviceNumber);
  int Write();
  int Read();
  ~DigitalInputOutputTask();
};

#endif // _DIGITALINPUTOUTPUTTASK_H_
