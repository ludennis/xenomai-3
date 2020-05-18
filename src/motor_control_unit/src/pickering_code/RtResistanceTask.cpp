#include <RtResistanceTask.h>

std::shared_ptr<RtSharedResistanceArray> RtResistanceTask::mRtSharedResistanceArray;

/* RtResistanceTask function definitions */
RtResistanceTask::RtResistanceTask()
{}

int RtResistanceTask::StartRoutine()
{
   // TODO: check if cardNum, device, bus has been set
   int e1 = rt_task_create(&mRtTask, "SetSubunitResistanceRoutine",
     kTaskStackSize, kMediumTaskPriority, kTaskMode);
   int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(kTaskPeriod));
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
      mData[0] = mRtSharedResistanceArray->Get(i);
      PIL_WriteSub(mCardNum, i, mData);

      rt_task_wait_period(NULL);

      mNow = rt_timer_read();

      if(static_cast<long>(mNow - mOneSecondTimer) / kNanosecondsToSeconds > 0)
      {
        printf("Time elapsed for task: %ld.%ld microseconds\n",
          static_cast<long>(mNow - mPrevious) / kNanosecondsToMicroseconds,
          static_cast<long>(mNow - mPrevious) % kNanosecondsToMicroseconds);
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
