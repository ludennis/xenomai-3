#include <RtGenerateStateTask.h>

std::shared_ptr<RtSharedState> RtGenerateStateTask::mRtSharedState;
testingModelClass RtGenerateStateTask::mModel;

RtGenerateStateTask::RtGenerateStateTask(
  const char* name, const int stackSize, const int priority, const int mode,
  const int period, const int coreId)
  : RtPeriodicTask(name, stackSize, priority, mode, period, coreId)
{}

int RtGenerateStateTask::StartRoutine()
{
  mlockall(MCL_CURRENT|MCL_FUTURE);

  int e1 = rt_task_create(&mRtTask, mName, mStackSize, mPriority, mMode);
  int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(mPeriod));
  int e3 = (mCoreId > 0) ? rt_task_set_affinity(&mRtTask, &mCpuSet) : 0;
  int e4 = rt_task_start(&mRtTask, &Routine, NULL);

  if(e1 | e2 | e3 | e4)
  {
    printf("Error launching RtGenerateStateTask::StartRoutine(). Exiting.\n");
    return -1;
  }
  printf("RtGenerateStateTask running on CoreId: %d\n", mCoreId);
}

void RtGenerateStateTask::Routine(void*)
{
  mModel.initialize();

  mOneSecondTimer = rt_timer_read();

  while(true)
  {
    mModel.step();

    mRtSharedState->Set(!mRtSharedState->Get());

    rt_task_wait_period(NULL);
  }
}

RtGenerateStateTask::~RtGenerateStateTask()
{
  mModel.terminate();
  printf("RtGenerateStateTask::mModel terminated\n");

  int e = rt_task_delete(&mRtTask);
  if(e)
  {
    printf("Error deleting RtGenerateStateTask::mRtTask. Exiting.\n");
    exit(-1);
  }
  printf("RtGenerateStateTask::mRtTask deleted\n");
}
