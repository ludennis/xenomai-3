#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <testing.h>

#include <RtTaskHandler.h>

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

  auto testingModel = testingModelClass();
  testingModel.initialize();
  for(auto i{0u}; i < 10u; ++i)
  {
    testingModel.step();
    std::cout << "Out1 after step(): " << testingModel.testing_Y.Out1 << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  testingModel.terminate();

  return 0;
}
