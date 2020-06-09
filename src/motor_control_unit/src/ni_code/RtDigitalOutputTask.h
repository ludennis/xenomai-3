#ifndef _RTDIGITALOUTPUTTASK_H_
#define _RTDIGITALOUTPUTTASK_H_

#include <RtPeriodicTask.h>

#include <DigitalInputOutputTask.h>

class RtDigitalOutputTask : public DigitalInputOutputTask, public RtPeriodicTask
{
public:
  RtDigitalOutputTask() = delete;
  RtDigitalOutputTask(
    const char *name, const int stackSize, const int priority, const int mode,
    const int period, const int coreId);

  int StartRoutine();
  static void Routine(void*);

  ~RtDigitalOutputTask();
};

#endif // _RTDIGITALOUTPUTTASK_H_
