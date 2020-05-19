#ifndef _RTPERIODICTASK_H_
#define _RTPERIODICTASK_H_

#include <alchemy/task.h>

class RtPeriodicTask
{
public:
  static RT_TASK mRtTask;
  static RTIME mNow;
  static RTIME mPrevious;
  static RTIME mOneSecondTimer;
  static const char* mName;
  static int mStackSize;
  static int mPriority;
  static int mMode;
  static int mPeriod;

public:
  RtPeriodicTask();
  RtPeriodicTask(
    const char* name, const int stackSize, const int priority, const int mode, const int period);
  int StartRoutine() = delete;
  static void Routine(void*) = delete;
};

#endif // _RTPERIODICTASK_H_
