#include <RtPeriodicTask.h>

/* static definition for RtTaskHandler */
RT_TASK RtPeriodicTask::mRtTask;
RTIME RtPeriodicTask::mNow;
RTIME RtPeriodicTask::mOneSecondTimer;
RTIME RtPeriodicTask::mPrevious;

RtPeriodicTask::RtPeriodicTask()
{}

int RtPeriodicTask::StartRoutine()
{
  return 0;
}

void RtPeriodicTask::Routine(void*)
{}
