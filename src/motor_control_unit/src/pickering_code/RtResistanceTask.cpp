#include <RtResistanceTask.h>

std::shared_ptr<RtSharedArray> RtResistanceTask::mRtSharedArray;

RtResistanceTask::RtResistanceTask(
  const char* name, const int stackSize, const int priority, const int mode, const int period)
  : RtPeriodicTask(name, stackSize, priority, mode, period)
{}

int RtResistanceTask::StartRoutine()
{
  int e1 = rt_task_create(&mRtTask, mName, mStackSize, mPriority, mMode);
  int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(mPeriod));
  int e3 = rt_task_start(&mRtTask, &Routine, NULL);

   if(e1 | e2 | e3)
   {
     printf("Error launching periodic task SetSubunitResistanceRoutine. Exiting.\n");
     return -1;
   }
}

void RtResistanceTask::Routine(void*)
{
  printf("Accessing bus %d, device %d, target resistance %d\n", mBus, mDevice, mResistance);
  mPrevious = rt_timer_read();
  while(true)
  {
    for(auto i{0u}; i < mNumOutputSubunits; ++i)
    {
      PIL_ViewSub(mCardNum, i, mData);
      auto previousResistance = mData[0];
      mData[0] = mRtSharedArray->Get(i);
      PIL_WriteSub(mCardNum, i, mData);

      rt_task_wait_period(NULL);

      mNow = rt_timer_read();

      if(static_cast<long>(mNow - mOneSecondTimer) / RtMacro::kNanosecondsToSeconds > 0)
      {
        printf("Time elapsed for task: %ld.%ld microseconds\n",
          static_cast<long>(mNow - mPrevious) / RtMacro::kNanosecondsToMicroseconds,
          static_cast<long>(mNow - mPrevious) % RtMacro::kNanosecondsToMicroseconds);
        mOneSecondTimer = mNow;

        /* show all subunits */
        PIL_EnumerateSubs(mCardNum, &mNumInputSubunits, &mNumOutputSubunits);
        for(auto i{0u}; i < mNumOutputSubunits; ++i)
        {
          BOOL out;
          CHAR subType[100];
          DWORD data[100];
          PIL_SubType(mCardNum, i, out, subType);
          PIL_ViewSub(mCardNum, i, data);
          printf("Subunit #%d (%s) = %d Ohm\n", i, subType, data[0]);
        }
        printf("\n");
      }

      mPrevious = mNow;
    }
  }
}

RtResistanceTask::~RtResistanceTask()
{
  int e = rt_task_delete(&mRtTask);
  if(e)
  {
    printf("Error deleting task RtResistanceTask::mRtTask. Exiting.\n");
    exit(-1);
  }
  printf("RtResistanceTask::mRtTask deleted\n");
}
