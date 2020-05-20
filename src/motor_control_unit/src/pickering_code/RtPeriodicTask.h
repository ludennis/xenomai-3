#ifndef _RTPERIODICTASK_H_
#define _RTPERIODICTASK_H_

#include <alchemy/task.h>

class RtPeriodicTask
{
public:
  RT_TASK mRtTask;

  static RTIME mNow;
  static RTIME mOneSecondTimer;
  static RTIME mPrevious;

  const char* mName;

  int mCoreId;
  int mMode;
  int mPeriod;
  int mPriority;
  int mStackSize;

  cpu_set_t mCpuSet;

public:
  RtPeriodicTask() = delete;
  RtPeriodicTask(
    const char* name, const int stackSize, const int priority, const int mode,
    const int period, const int coreId);
  int StartRoutine() = delete;
  static void Routine(void*) = delete;
};

#endif // _RTPERIODICTASK_H_
