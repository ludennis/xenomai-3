#include <RtPeriodicTask.h>

/* static definition for RtTaskHandler */
RT_TASK RtPeriodicTask::mRtTask;
RTIME RtPeriodicTask::mNow;
RTIME RtPeriodicTask::mOneSecondTimer;
RTIME RtPeriodicTask::mPrevious;
const char* RtPeriodicTask::mName;
int RtPeriodicTask::mStackSize;
int RtPeriodicTask::mPriority;
int RtPeriodicTask::mMode;
int RtPeriodicTask::mPeriod;

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
