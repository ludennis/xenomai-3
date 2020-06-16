#ifndef _RTDMAINPUTOUTPUTTASK_H_
#define _RTDMAINPUTOUTPUTTASK_H_

#include <DmaDigitalInputOutputTask.h>
#include <RtPeriodicTask.h>

class RtDmaDigitalInputOutputTask : public DmaDigitalInputOutputTask, public RtPeriodicTask
{
public:
  RtDmaDigitalInputOutputTask() = delete;
  RtDmaDigitalInputOutputTask(const char *name, const int stackSize, const int priority,
    const int mode, const int period, const int coreId);
  void StartRoutine();
  static void Routine(void*);
  ~RtDmaDigitalInputOutputTask();
};

#endif // _RTDMAINPUTOUTPUTTASK_H_
