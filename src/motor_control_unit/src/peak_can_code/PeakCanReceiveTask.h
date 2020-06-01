#ifndef _PEAKCANRECEIVETASK_H_
#define _PEAKCANRECEIVETASK_H_

#include <PeakCanTask.h>

class PeakCanReceiveTask : public PeakCanTask
{
protected:
  TPCANRdMsg mReadMessage;

public:
  PeakCanReceiveTask() = delete;
  PeakCanReceiveTask(const char *deviceName, const unsigned int baudRate);

  int Read();
};

#endif // _PEAKCANRECEIVETASK_H_
