#include <RtPeriodicTask.h>

RTIME RtPeriodicTask::mNow;
RTIME RtPeriodicTask::mOneSecondTimer;
RTIME RtPeriodicTask::mPrevious;

RtPeriodicTask::RtPeriodicTask(
  const char* name, const int stackSize, const int priority, const int mode,
  const int period, const int coreId)
{
  mName = name;
  mStackSize = stackSize;
  mPriority = priority;
  mMode = mode;
  mPeriod = period;
  mCoreId = coreId;

  CPU_ZERO(&mCpuSet);
  CPU_SET(mCoreId, &mCpuSet);
}
