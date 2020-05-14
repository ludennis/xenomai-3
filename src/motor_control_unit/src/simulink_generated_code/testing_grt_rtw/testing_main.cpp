#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <testing.h>

#include <alchemy/mutex.h>

#include <RtTaskHandler.h>

static auto sharedResistanceArray = std::make_shared<SharedResistanceArray>();

static auto rtTaskHandler = std::make_unique<RtTaskHandler>();

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

  DWORD cardNum = 3;
  rtTaskHandler->OpenCard(cardNum);
  rtTaskHandler->mSharedResistanceArray = sharedResistanceArray;

  printf("test: %d", rtTaskHandler->mSharedResistanceArray->Get(1));

  rtTaskHandler->StartSetSubunitResistanceRoutine();


  auto testingModel = testingModelClass();
  testingModel.initialize();

  std::vector<DWORD> resistances;
  while(true)
  {
    resistances.clear();

    for(auto i{0u}; i < 10u; ++i)
    {
      testingModel.step();

      DWORD subunit = i;
      DWORD resistance = testingModel.testing_Y.Out1 * 2+ 10;

      resistances.push_back(resistance);
    }

    sharedResistanceArray->SetArray(resistances);

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    testingModel.terminate();
  }
  return 0;
}
