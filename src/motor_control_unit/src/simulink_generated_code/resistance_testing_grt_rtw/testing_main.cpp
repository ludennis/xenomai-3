#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <alchemy/mutex.h>

#include <testing.h>

#include <RtMacro.h>
#include <RtResistanceTask.h>

RT_TASK rtResistanceArrayTask;

static auto rtSharedResistanceArray = std::make_shared<RtSharedResistanceArray>();
static auto rtResistanceTask = std::make_unique<RtResistanceTask>();
// TODO: move this into an RT task
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

    rtSharedResistanceArray->SetArray(resistances);

    rt_task_wait_period(NULL);
  }
}

int StartGenerateResistanceArrayRoutine()
{
  int e1 = rt_task_create(&rtResistanceArrayTask, "GenerateResistanceArrayRoutine",
    RtMacro::kTaskStackSize, RtMacro::kMediumTaskPriority, RtMacro::kTaskMode);
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
  PIL_ClearCard(rtResistanceTask->mCardNum);
  PIL_CloseSpecifiedCard(rtResistanceTask->mCardNum);
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
  rtResistanceTask->OpenCard(cardNum);
  rtResistanceTask->mRtSharedResistanceArray = rtSharedResistanceArray;
  rtResistanceTask->StartRoutine();

  while(true) // original parent process will wait until ctrl+c signal
  {}

  return 0;
}
