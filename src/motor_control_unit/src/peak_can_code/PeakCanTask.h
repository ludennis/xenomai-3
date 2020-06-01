#ifndef _PEAKCANTASK_H_
#define _PEAKCANTASK_H_

#include <errno.h>
#include <stdio.h>

#include <libpcan.h>

class PeakCanTask
{
protected:
  static HANDLE mHandle;
  unsigned int mBaudRate;

public:
  PeakCanTask() = delete;
  PeakCanTask(const char *deviceName, const unsigned int baudRate);

  int Init();

  ~PeakCanTask();
};

#endif // _PEAKCANTASK_H_
