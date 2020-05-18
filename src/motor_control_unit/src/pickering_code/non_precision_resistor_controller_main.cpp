#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <Pilpxi.h>

#include <RtTaskHandler.h>

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
      rtTaskHandler->StartSetSubunitResistanceRoutine();

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
