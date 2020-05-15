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
DWORD RtTaskHandler::mResistances[100];
DWORD RtTaskHandler::mSubunit;

CHAR RtTaskHandler::mCardId[100];

std::shared_ptr<SharedResistanceArray> RtTaskHandler::mSharedResistanceArray;

/* RtTaskHandler function definitions */
RtTaskHandler::RtTaskHandler()
{}

void RtTaskHandler::OpenCard(DWORD cardNum)
{
  PIL_CountFreeCards(&mNumOfFreeCards);
  PIL_FindFreeCards(mNumOfFreeCards, mBuses, mDevices);
  PIL_OpenSpecifiedCard(mBuses[cardNum-1], mDevices[cardNum-1], &mCardNum);
  mBus = mBuses[mCardNum-1];
  mDevice = mDevices[mCardNum-1];
  PIL_CardId(mCardNum, mCardId);
  PIL_EnumerateSubs(mCardNum, &mNumInputSubunits, &mNumOutputSubunits);

  printf("Opening cardNum: %d, mCardNum: %d, bus: %d, device: %d, card id: %s, "
    "# input subunits: %d, # output subunits: %d\n",
    cardNum, mCardNum, mBus, mDevice, mCardId, mNumInputSubunits, mNumOutputSubunits);
}

int RtTaskHandler::StartSetSubunitResistanceRoutine()
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

void RtTaskHandler::ViewAllSubunits(DWORD cardNum)
{
  PIL_EnumerateSubs(mCardNum, &mNumInputSubunits, &mNumOutputSubunits);
  for(auto i{0u}; i < mNumOutputSubunits; ++i)
  {
    BOOL out;
    CHAR subType[100];
    DWORD data[100];
    PIL_SubType(cardNum, i, out, subType);
    PIL_ViewSub(cardNum, i, data);

    printf("Subunit #%d (%s) = %d Ohm", i, subType, data[0]);
  }
}

void RtTaskHandler::SetSubunitResistanceRoutine(void*)
{
  printf("Accessing bus %d, device %d, target resistance %d\n", mBus, mDevice, mResistance);
  mPrevious = rt_timer_read();
  while(true)
  {
    for(auto i{0u}; i < mNumOutputSubunits; ++i)
    {
      PIL_ViewSub(mCardNum, i, mData);
      auto previousResistance = mData[0];
      mData[0] = mSharedResistanceArray->Get(i);
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
