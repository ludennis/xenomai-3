#ifndef _PEAKCANTRANSMITTASK_H_
#define _PEAKCANTRANSMITTASK_H_

#include <errno.h>
#include <stdio.h>

#include <libpcan.h>

class PeakCanTransmitTask
{
private:
  HANDLE mHandle;
  unsigned int mBaudRate;

public:
  PeakCanTransmitTask() = delete;
  PeakCanTransmitTask(const char *deviceName, const unsigned int baudRate);

  int Init();
  int Write(TPCANMsg &writeMessage);

  ~PeakCanTransmitTask();
};

#endif // _PEAKCANTRANSMITTASK_H_
