#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <alchemy/task.h>

#include <Pilpxi.h>

constexpr auto kTaskStackSize = 0;
constexpr auto kMediumTaskPriority = 50;
constexpr auto kTaskMode = 0;
constexpr auto kTaskPeriod = 20000; // 20 us
constexpr auto kNanosecondsToMicroseconds = 1000;
constexpr auto kNanosecondsToMilliseconds = 1000000;
constexpr auto kNanosecondsToSeconds = 1000000000;

class RtTaskHandler
{
public:
  static RT_TASK mRtTask;
  static RTIME mNow;
  static RTIME mPrevious;
  static RTIME mOneSecondTimer;

  static DWORD mBus;
  static DWORD mBuses[100];
  static DWORD mDevices[100];
  static DWORD mDevice;
  static DWORD mCardNum;
  static DWORD mData[100];
  static DWORD mSubunit;
  static DWORD mResistance;
  static DWORD mNumInputSubunits;
  static DWORD mNumOutputSubunits;
  static DWORD mNumOfFreeCards;

public:
  RtTaskHandler(){}

  int SetSubunitResistance()
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

  static void SetSubunitResistanceRoutine(void*)
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
};

/* static definition for RtTaskHandler */
DWORD RtTaskHandler::mNumOfFreeCards;
DWORD RtTaskHandler::mBuses[100];
DWORD RtTaskHandler::mDevices[100];
DWORD RtTaskHandler::mResistance;
DWORD RtTaskHandler::mDevice;
DWORD RtTaskHandler::mBus;
DWORD RtTaskHandler::mSubunit;
DWORD RtTaskHandler::mCardNum;
DWORD RtTaskHandler::mNumInputSubunits;
DWORD RtTaskHandler::mNumOutputSubunits;
DWORD RtTaskHandler::mData[100];

RT_TASK RtTaskHandler::mRtTask;

RTIME RtTaskHandler::mPrevious;
RTIME RtTaskHandler::mNow;
RTIME RtTaskHandler::mOneSecondTimer;

/* initialize rt task handler for rt tasks */
auto rtTaskHandler = std::make_unique<RtTaskHandler>();

static CHAR id[100];
static DWORD err;

void TerminationHandler(int s)
{
  printf("Caught ctrl + c signal. Closing Card and Exiting.\n");
  PIL_ClearCard(rtTaskHandler->mCardNum);
  PIL_CloseSpecifiedCard(rtTaskHandler->mCardNum);
  exit(1);
}

int main(int argc, char **argv)
{
  // ctrl + c signal handler
  struct sigaction signalHandler;
  signalHandler.sa_handler = TerminationHandler;
  sigemptyset(&signalHandler.sa_mask);
  signalHandler.sa_flags = 0;
  sigaction(SIGINT, &signalHandler, NULL);

  if(argc == 1)
  {
    // find free cards
    PIL_CountFreeCards(&(rtTaskHandler->mNumOfFreeCards));
    printf("Found %d free cards\n", rtTaskHandler->mNumOfFreeCards);
    PIL_FindFreeCards(
      rtTaskHandler->mNumOfFreeCards, rtTaskHandler->mBuses, rtTaskHandler->mDevices);
    printf("Card addresses:\n");
    for(auto i{0}; i < rtTaskHandler->mNumOfFreeCards; ++i)
    {
      printf("Opening card at bus %d, device %d\n",
        rtTaskHandler->mBuses[i], rtTaskHandler->mDevices[i]);
      err = PIL_OpenSpecifiedCard(
        rtTaskHandler->mBuses[i], rtTaskHandler->mDevices[i], &(rtTaskHandler->mCardNum));
      if(err == 0)
      {
        printf("Card Number is %d\n", rtTaskHandler->mCardNum);
        PIL_CardId(rtTaskHandler->mCardNum, id);
        printf(" Card id is %s\n", id);
        PIL_CloseSpecifiedCard(rtTaskHandler->mCardNum);
      }
      else
      {
        printf("Error return is %d\n", err);
      }
    }
  }
  else if(argc == 5)
  {
    rtTaskHandler->mBus = std::atoi(argv[1]);
    rtTaskHandler->mDevice = std::atoi(argv[2]);
    rtTaskHandler->mSubunit = std::atoi(argv[3]);
    rtTaskHandler->mResistance = std::atoi(argv[4]);

    // open card
    err = PIL_OpenSpecifiedCard(
      rtTaskHandler->mBus, rtTaskHandler->mDevice, &(rtTaskHandler->mCardNum));

    if(err == 0)
    {
      printf("Card Number is %d\n", rtTaskHandler->mCardNum);
      PIL_EnumerateSubs(
        rtTaskHandler->mCardNum, &(rtTaskHandler->mNumInputSubunits),
        &(rtTaskHandler->mNumOutputSubunits));
      PIL_CardId(rtTaskHandler->mCardNum, id);
      printf("Card id is %s, number of input subunits: %d, number of output subunits: %d\n",
        id, rtTaskHandler->mNumInputSubunits, rtTaskHandler->mNumOutputSubunits);

      // check if the card is a resistor
      std::string cardId(id);
      std::string resistorId("40-295-121");
      if(cardId.find(resistorId) != std::string::npos)
      {
        printf("it has resistor id(%s)!\n", resistorId.c_str());
      }
      else
      {
        printf("Card doesn't have resistor id (%s), might not be a resistor card\n",
          resistorId.c_str());
        return -1;
      }

      rtTaskHandler->mOneSecondTimer = rt_timer_read();
      rtTaskHandler->SetSubunitResistance();

      /* runs until ctrl+c termination signal is received */
      while(true)
      {}
    }
    else
    {
      printf("Error opening card, error %d\n", err);
      return -1;
    }
  }
  else
  {
    printf("usage: non_precision_resistor_controller [bus] [device] [subunit] [resistance]\n");
  }

  return 0;
}
