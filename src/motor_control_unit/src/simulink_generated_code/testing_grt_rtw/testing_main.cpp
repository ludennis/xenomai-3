#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <testing.h>

#include <alchemy/mutex.h>

#include <RtTaskHandler.h>

RT_TASK rtResistanceArrayTask;

static auto sharedResistanceArray = std::make_shared<SharedResistanceArray>();
static auto rtTaskHandler = std::make_unique<RtTaskHandler>();
static auto testingModel = testingModelClass();

void GenerateResistanceArrayRoutine(void*)
{
  std::vector<DWORD> resistances;
  testingModel.initialize();

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

    rt_task_wait_period(NULL);
  }
}

int StartGenerateResistanceArrayRoutine()
{
  int e1 = rt_task_create(&rtResistanceArrayTask, "GenerateResistanceArrayRoutine",
    kTaskStackSize, kMediumTaskPriority, kTaskMode);
  int e2 = rt_task_set_periodic(&rtResistanceArrayTask, TM_NOW, rt_timer_ns2ticks(1000000));
  int e3 = rt_task_start(&rtResistanceArrayTask, &GenerateResistanceArrayRoutine, NULL);

  if(e1 | e2 | e3)
  {
    printf("Error launching StartSetSubunitResistanceRoutine. Exiting.\n");
    return -1;
  }
}

void TerminationHandler(int s)
{
  printf("Caught ctrl + c signal. Closing Card and Exiting.\n");
  PIL_ClearCard(rtTaskHandler->mCardNum);
  PIL_CloseSpecifiedCard(rtTaskHandler->mCardNum);
  testingModel.terminate();

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

  StartGenerateResistanceArrayRoutine();

  DWORD cardNum = 3;
  rtTaskHandler->OpenCard(cardNum);
  rtTaskHandler->mSharedResistanceArray = sharedResistanceArray;
  rtTaskHandler->StartSetSubunitResistanceRoutine();

  while(true) // original parent process will wait until ctrl+c signal
  {}

  return 0;
}
