#include <RtTaskHandler.h>

/* static definition for RtTaskHandler */
RT_TASK RtTaskHandler::mRtTask;
RTIME RtTaskHandler::mNow;
RTIME RtTaskHandler::mOneSecondTimer;
RTIME RtTaskHandler::mPrevious;

DWORD RtTaskHandler::mBus;
DWORD RtTaskHandler::mBuses[100];
DWORD RtTaskHandler::mCardNum;
DWORD RtTaskHandler::mData[100];
DWORD RtTaskHandler::mDevices[100];
DWORD RtTaskHandler::mDevice;
DWORD RtTaskHandler::mNumInputSubunits;
DWORD RtTaskHandler::mNumOfFreeCards;
DWORD RtTaskHandler::mNumOutputSubunits;
DWORD RtTaskHandler::mResistance;
DWORD RtTaskHandler::mSubunit;

/* RtTaskHandler function definitions */
int RtTaskHandler::SetSubunitResistance()
{
   // TODO: check if cardNum, device, bus has been set
   int e1 = rt_task_create(&mRtTask, "SetSubunitResistanceRoutine",
     kTaskStackSize, kMediumTaskPriority, kTaskMode);
   int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(kTaskPeriod));
   int e3 = rt_task_start(&mRtTask, &SetSubunitResistanceRoutine, NULL);

   if(e1 | e2 | e3)
   {
     printf("Error launching periodic task SetSubunitResistanceRoutine. Exiting.\n");
     return -1;
   }
}

void RtTaskHandler::SetSubunitResistanceRoutine(void*)
{
  printf("Accessing bus %d, device %d, target resistance %d\n", mBus, mDevice, mResistance);
  // change the resistance
  //RTIME now, previous;
  mPrevious = rt_timer_read();
  while(mResistance > 0.0)
  {
    for(auto i{0u}; i < mNumOutputSubunits; ++i)
    {
      PIL_ReadSub(mCardNum, i, mData);
      auto previousResistance = mData[0];
      mData[0] = mResistance;
      PIL_WriteSub(mCardNum, mSubunit, mData);
      //printf("Subunit #%d's resistance changed from %d -> %d\n",
      //  i, previousResistance, mResistance);
      rt_task_wait_period(NULL);
      mNow = rt_timer_read();
      if(static_cast<long>(mNow - mOneSecondTimer) / kNanosecondsToSeconds > 0)
      {
        printf("Time elapsed for task: %ld.%ld microseconds\n",
          static_cast<long>(mNow - mPrevious) / kNanosecondsToMicroseconds,
          static_cast<long>(mNow - mPrevious) % kNanosecondsToMicroseconds);
        mOneSecondTimer = mNow;
      }
      mPrevious = mNow;
    }
  }
}
