#include <RtGenerateResistanceArrayTask.h>

// TODO: change name of RtsharedResistanceArray to something more generic
std::shared_ptr<RtSharedArray> RtGenerateResistanceArrayTask::mRtSharedArray;
testingModelClass RtGenerateResistanceArrayTask::mModel;

RtGenerateResistanceArrayTask::RtGenerateResistanceArrayTask(
  const char* name, const int stackSize, const int priority, const int mode,
  const int period, const int coreId)
  : RtPeriodicTask(name, stackSize, priority, mode, period, coreId)
{}

int RtGenerateResistanceArrayTask::StartRoutine()
{
  mlockall(MCL_CURRENT|MCL_FUTURE);

  int e1 = rt_task_create(&mRtTask, mName, mStackSize, mPriority, mMode);
  int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(mPeriod));
  int e3 = (mCoreId > 0) ? rt_task_set_affinity(&mRtTask, &mCpuSet) : 0;
  int e4 = rt_task_start(&mRtTask, &Routine, NULL);

  if(e1 | e2 | e3 | e4)
  {
    printf("Error with RtGenerateResistanceArrayTask::StartRoutine(). Exiting.\n");
    exit(-1);
  }
}

void RtGenerateResistanceArrayTask::Routine(void*)
{
  mModel.initialize();

  std::vector<DWORD> resistances;

  while(true)
  {
    resistances.clear();

    // TODO: make 10u a size for share array
    for(auto i{0u}; i < 10u; ++i)
    {
      mModel.step();
      DWORD subunit = i;
      DWORD resistance = mModel.testing_Y.Out1 * 2 + 10;

      resistances.push_back(resistance);
    }
    mRtSharedArray->SetArray(resistances);

    rt_task_wait_period(NULL);
  }
}

RtGenerateResistanceArrayTask::~RtGenerateResistanceArrayTask()
{
  mModel.terminate();
  printf("RtGenerateResistanceArrayTask::mModel terminated\n");
}
