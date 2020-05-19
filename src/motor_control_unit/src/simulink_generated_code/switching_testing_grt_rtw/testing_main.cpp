#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <testing.h>

#include <alchemy/mutex.h>

#include <RtSharedState.h>
#include <RtSwitchTask.h>

RT_TASK rtSwitchStateTask;
static auto rtSwitchTask = std::make_unique<RtSwitchTask>();
static auto rtSharedState = std::make_shared<RtSharedState>();

static auto testingModel = testingModelClass();

// TODO: make this another task inherited from RtPeriodicTask
void GenerateSwitchStateRoutine(void*)
{
  testingModel.initialize();

  while(true)
  {
    testingModel.step();

    rtSharedState->Set(!rtSharedState->Get());

    printf("Generated state: %s\n", rtSharedState->Get() ? "true" : "false");

    rt_task_wait_period(NULL);
  }
}

int StartGenerateSwitchStateRoutine()
{
  int e1 = rt_task_create(&rtSwitchStateTask, "GenerateSwitchStateRoutine",
    RtMacro::kTaskStackSize, RtMacro::kMediumTaskPriority, RtMacro::kTaskMode);
  int e2 = rt_task_set_periodic(
    &rtSwitchStateTask, TM_NOW, rt_timer_ns2ticks(RtMacro::kHundredMsTaskPeriod));
  int e3 = rt_task_start(&rtSwitchStateTask, &GenerateSwitchStateRoutine, NULL);

  if(e1 | e2 | e3)
  {
    printf("Error launching StartGenerateSwitchStateRoutine(). Exiting.\n");
  }
}

void TerminationHandler(int s)
{
  printf("Caught ctrl + c signal. Closing Card and Exiting.\n");

// TODO: make close card a destructor for PxiCardTask
  PIL_ClearCard(rtSwitchTask->mCardNum);
  PIL_CloseSpecifiedCard(rtSwitchTask->mCardNum);
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

  StartGenerateSwitchStateRoutine();

  DWORD cardNum = 1;
  DWORD subunit = 1;
  DWORD bit = 1;
  rtSwitchTask->OpenCard(cardNum);
  rtSwitchTask->mSubunit = subunit;
  rtSwitchTask->mBit = bit;
  rtSwitchTask->mRtSharedState = rtSharedState;
  rtSwitchTask->StartRoutine();

  while(true) // original parent process will wait until ctrl+c signal
  {}

  return 0;
}
