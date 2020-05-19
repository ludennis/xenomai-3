#include <RtSwitchTask.h>

std::shared_ptr<RtSharedState> RtSwitchTask::mRtSharedState;

RtSwitchTask::RtSwitchTask()
{}

int RtSwitchTask::StartRoutine()
{
  int e1 = rt_task_create(&mRtTask, "SetSubunitSwitchState",
    RtMacro::kTaskStackSize, RtMacro::kMediumTaskPriority, RtMacro::kTaskMode);
  int e2 = rt_task_set_periodic(
    &mRtTask, TM_NOW, rt_timer_ns2ticks(RtMacro::kTenMsTaskPeriod));
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

    printf("State changed from %s -> %s (setState = %s)\n",
      prevState ? "true" : "false", mState ? "true" : "false", setState ? "true" : "false");

    rt_task_wait_period(NULL);
  }
}
