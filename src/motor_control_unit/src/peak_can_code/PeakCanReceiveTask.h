#ifndef _PEAKCANRECEIVETASK_H_
#define _PEAKCANRECEIVETASK_H_

#include <PeakCanTask.h>

class PeakCanReceiveTask : public PeakCanTask
{
private:
  TPCANRdMsg mReadMessage;

public:
  PeakCanReceiveTask() = delete;
  PeakCanReceiveTask(const char *deviceName, const unsigned int baudRate);

  void PrintReadMessage();
  int Read();
};

#endif // _PEAKCANRECEIVETASK_H_
