#include <RtSwitchTask.h>

std::shared_ptr<RtSharedState> RtSwitchTask::mRtSharedState;

RtSwitchTask::RtSwitchTask(
  const char* name, const int stackSize, const int priority, const int mode, const int period)
  : RtPeriodicTask(name, stackSize, priority, mode, period)
{}

int RtSwitchTask::StartRoutine()
{
  int e1 = rt_task_create(&mRtTask, mName, mStackSize, mPriority, mMode);
  int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(mPeriod));
  int e3 = rt_task_start(&mRtTask, &Routine, NULL);

  if(e1 | e2 | e3)
  {
    printf("Error launching periodic task SetSubunitSwitchState. Exiting.\n");
    return -1;
  }
}

void RtSwitchTask::Routine(void*)
{
  printf("Accessing bus %d, device %d, target resistance %d\n", mBus, mDevice, mResistance);
  mPrevious = rt_timer_read();
  BOOL prevState;
  while(true)
  {
    // TODO: check how many bits are in the subunit
    PIL_ViewBit(mCardNum, mSubunit, mBit, &prevState);

    auto setState = mRtSharedState->Get();

    PIL_OpBit(mCardNum, mSubunit, mBit, setState);
    PIL_ViewBit(mCardNum, mSubunit, mBit, &mState);

    rt_task_wait_period(NULL);

    mNow = rt_timer_read();

    if(static_cast<long>(mNow - mOneSecondTimer) / RtMacro::kNanosecondsToSeconds > 0)
    {
      printf("Time elapsed for task: %ld.%ld microseconds\n",
        static_cast<long>(mNow - mPrevious) / RtMacro::kNanosecondsToMicroseconds,
        static_cast<long>(mNow - mPrevious) % RtMacro::kNanosecondsToMicroseconds);

      printf("State changed from %s -> %s (setState = %s)\n",
        prevState ? "true" : "false", mState ? "true" : "false", setState ? "true" : "false");

      mOneSecondTimer = mNow;
    }
    mPrevious = mNow;
  }
}
