#ifndef _RTDIGITALINPUTTASK_H_
#define _RTDIGITALINPUTTASK_H_

#include <RtPeriodicTask.h>

#include <DigitalInputOutputTask.h>

class RtDigitalInputTask : public DigitalInputOutputTask, public RtPeriodicTask
{
public:
  RtDigitalInputTask() = delete;
  RtDigitalInputTask(
    const char *name, const int stackSize, const int priority, const int mode,
    const int period, const int coreId);

  int StartRoutine();
  static void Routine(void*);

  ~RtDigitalInputTask();
};

#endif // _RTDIGITALINPUTTASK_H_
