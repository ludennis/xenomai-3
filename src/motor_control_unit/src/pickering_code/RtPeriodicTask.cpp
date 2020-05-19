#include <RtPeriodicTask.h>

RTIME RtPeriodicTask::mNow;
RTIME RtPeriodicTask::mOneSecondTimer;
RTIME RtPeriodicTask::mPrevious;

RtPeriodicTask::RtPeriodicTask()
{}

RtPeriodicTask::RtPeriodicTask(
  const char* name, const int stackSize, const int priority, const int mode, const int period)
{
  mName = name;
  mStackSize = stackSize;
  mPriority = priority;
  mMode = mode;
  mPeriod = period;
}
