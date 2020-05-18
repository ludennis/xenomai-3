#include <RtSwitchTask.h>

std::shared_ptr<RtSharedState> RtSwitchTask::mRtSharedState;

RtSwitchTask::RtSwitchTask()
{}

int RtSwitchTask::StartRoutine()
{
  int e1 = rt_task_create(&mRtTask, "SetSubunitSwitchState",
    RtMacro::kTaskStackSize, RtMacro::kMediumTaskPriority, RtMacro::kTaskMode);
  int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(RtMacro::kTaskPeriod));
  int e3 = rt_task_start(&mRtTask, &Routine, NULL);

  if(e1 | e2 | e3)
  {
    printf("Error launching periodic task SetSubunitSwitchState. Exiting.\n");
    return -1;
  }
}

void RtSwitchTask::Routine(void*)
{
}
