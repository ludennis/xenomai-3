#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <Pilpxi.h>

#include <RtResistanceTask.h>

/* initialize rt task handler for rt tasks */
auto rtResistanceTask = std::make_unique<RtResistanceTask>();

static CHAR id[100];
static DWORD err;

void TerminationHandler(int s)
{
  printf("Caught ctrl + c signal. Closing Card and Exiting.\n");
  PIL_ClearCard(rtResistanceTask->mCardNum);
  PIL_CloseSpecifiedCard(rtResistanceTask->mCardNum);
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
    PIL_CountFreeCards(&(rtResistanceTask->mNumOfFreeCards));
    printf("Found %d free cards\n", rtResistanceTask->mNumOfFreeCards);
    PIL_FindFreeCards(
      rtResistanceTask->mNumOfFreeCards, rtResistanceTask->mBuses, rtResistanceTask->mDevices);
    printf("Card addresses:\n");
    for(auto i{0}; i < rtResistanceTask->mNumOfFreeCards; ++i)
    {
      printf("Opening card at bus %d, device %d\n",
        rtResistanceTask->mBuses[i], rtResistanceTask->mDevices[i]);
      err = PIL_OpenSpecifiedCard(
        rtResistanceTask->mBuses[i], rtResistanceTask->mDevices[i], &(rtResistanceTask->mCardNum));
      if(err == 0)
      {
        printf("Card Number is %d\n", rtResistanceTask->mCardNum);
        PIL_CardId(rtResistanceTask->mCardNum, id);
        printf(" Card id is %s\n", id);
        PIL_CloseSpecifiedCard(rtResistanceTask->mCardNum);
      }
      else
      {
        printf("Error return is %d\n", err);
      }
    }
  }
  else if(argc == 5)
  {
    rtResistanceTask->mBus = std::atoi(argv[1]);
    rtResistanceTask->mDevice = std::atoi(argv[2]);
    rtResistanceTask->mSubunit = std::atoi(argv[3]);
    rtResistanceTask->mResistance = std::atoi(argv[4]);

    // open card
    err = PIL_OpenSpecifiedCard(
      rtResistanceTask->mBus, rtResistanceTask->mDevice, &(rtResistanceTask->mCardNum));

    if(err == 0)
    {
      printf("Card Number is %d\n", rtResistanceTask->mCardNum);
      PIL_EnumerateSubs(
        rtResistanceTask->mCardNum, &(rtResistanceTask->mNumInputSubunits),
        &(rtResistanceTask->mNumOutputSubunits));
      PIL_CardId(rtResistanceTask->mCardNum, id);
      printf("Card id is %s, number of input subunits: %d, number of output subunits: %d\n",
        id, rtResistanceTask->mNumInputSubunits, rtResistanceTask->mNumOutputSubunits);

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

      rtResistanceTask->mOneSecondTimer = rt_timer_read();
      rtResistanceTask->StartSetSubunitResistanceRoutine();

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
