#include <PxiCardTask.h>

DWORD PxiCardTask::mBus;
DWORD PxiCardTask::mBuses[100];
DWORD PxiCardTask::mCardNum;
DWORD PxiCardTask::mData[100];
DWORD PxiCardTask::mDevices[100];
DWORD PxiCardTask::mDevice;
DWORD PxiCardTask::mNumInputSubunits;
DWORD PxiCardTask::mNumOfFreeCards;
DWORD PxiCardTask::mNumOutputSubunits;
DWORD PxiCardTask::mResistance;
DWORD PxiCardTask::mResistances[100];
DWORD PxiCardTask::mSubunit;
CHAR PxiCardTask::mCardId[100];

PxiCardTask::PxiCardTask()
{}

void PxiCardTask::OpenCard(DWORD cardNum)
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

void PxiCardTask::ViewAllSubunits(DWORD cardNum)
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
