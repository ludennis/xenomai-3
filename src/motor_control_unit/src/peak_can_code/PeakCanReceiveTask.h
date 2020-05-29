#ifndef _PEAKCANRECEIVETASK_H_
#define _PEAKCANRECEIVETASK_H_

#include <errno.h>
#include <stdio.h>

#include <libpcan.h>

class PeakCanReceiveTask
{
private:
  HANDLE mHandle;
  TPCANRdMsg mReadMessage;
  unsigned int mBaudRate;

public:
  PeakCanReceiveTask() = delete;
  PeakCanReceiveTask(const char *deviceName, const unsigned int baudRate);

  int Init();
  void PrintMessage(const char *prompt, TPCANMsg *m);
  void PrintReadMessage();
  int Read();

  ~PeakCanReceiveTask();
};

#endif // _PEAKCANRECEIVETASK_H_
