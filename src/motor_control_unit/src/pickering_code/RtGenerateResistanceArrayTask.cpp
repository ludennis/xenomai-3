#include <RtGenerateResistanceArrayTask.h>

// TODO: change name of RtsharedResistanceArray to something more generic
std::shared_ptr<RtSharedResistanceArray> RtGenerateResistanceArrayTask::mRtSharedResistanceArray;
testingModelClass RtGenerateResistanceArrayTask::mModel;

RtGenerateResistanceArrayTask::RtGenerateResistanceArrayTask()
{}

RtGenerateResistanceArrayTask::RtGenerateResistanceArrayTask(
  const char* name, const int stackSize, const int priority, const int mode, const int period)
{
  mName = name;
  mStackSize = stackSize;
  mPriority = priority;
  mMode = mode;
  mPeriod = period;
}

int RtGenerateResistanceArrayTask::StartRoutine()
{
  int e1 = rt_task_create(&mRtTask, mName, mStackSize, mPriority, mMode);
  int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(mPeriod));
  int e3 = rt_task_start(&mRtTask, &Routine, NULL);

  if(e1 | e2 | e3)
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
    mRtSharedResistanceArray->SetArray(resistances);

    rt_task_wait_period(NULL);
  }
}

RtGenerateResistanceArrayTask::~RtGenerateResistanceArrayTask()
{
  // TODO: check if it's actually called
  mModel.terminate();
}
