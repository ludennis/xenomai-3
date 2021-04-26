#include <PxiCardTask.h>

DWORD PxiCardTask::mBit;
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
DWORD PxiCardTask::mTypeNum;
DWORD PxiCardTask::mRows;
DWORD PxiCardTask::mCols;

CHAR PxiCardTask::mCardId[100];

BOOL PxiCardTask::mOut;
BOOL PxiCardTask::mState;

PxiCardTask::PxiCardTask()
{}

void PxiCardTask::ListAllCards()
{
  PIL_CountFreeCards(&mNumOfFreeCards);
  PIL_FindFreeCards(mNumOfFreeCards, mBuses, mDevices);

  for (auto cardNum{0}; cardNum < mNumOfFreeCards; ++cardNum)
  {
    PIL_OpenSpecifiedCard(mBuses[cardNum], mDevices[cardNum], &mCardNum);
    PIL_EnumerateSubs(mCardNum, &mNumInputSubunits, &mNumOutputSubunits);

    printf("Card #: %d, bus: %d, device: %d\n\n", cardNum, mBuses[cardNum], mDevices[cardNum]);

    for (auto inputSubNum{0}; inputSubNum < mNumInputSubunits; ++inputSubNum)
    {
      PIL_SubInfo(cardNum, inputSubNum, mOut, &mTypeNum, &mRows, &mCols);
      printf("Input subunit #: %d, subunit type: %d", inputSubNum, mTypeNum);
    }

    for (auto outputSubNum{0}; outputSubNum < mNumOutputSubunits; ++outputSubNum)
    {
      PIL_SubInfo(cardNum, outputSubNum, mOut, &mTypeNum, &mRows, &mCols);
      printf("Output subunit #: %d, subunit type: %d", outputSubNum, mTypeNum);
    }
  }

  PIL_CloseCards();
}

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
