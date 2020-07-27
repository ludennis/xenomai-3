#ifndef _RTPEAKCANRECEIVETASK_H_
#define _RTPEAKCANRECEIVETASK_H_

#include <stdlib.h>
#include <sys/mman.h>

#include <PeakCanTask.h>
#include <RtPeriodicTask.h>

class RtPeakCanReceiveTask : public PeakCanTask, public RtPeriodicTask
{
public:
  RtPeakCanReceiveTask() = delete;
  RtPeakCanReceiveTask(const char *deviceName, const unsigned int baudRate,
    const char *name, const int stackSize, const int priority, const int mode,
    const int period, const int coreId);

  int StartRoutine();
  static void Routine(void*);

  ~RtPeakCanReceiveTask();
};

#endif // _RTPEAKCANRECEIVETASK_H_
