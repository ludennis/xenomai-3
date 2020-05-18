#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <testing.h>

#include <alchemy/mutex.h>

#include <RtResistanceTask.h>

class RtSharedState
{
public:
  BOOL mState;
  RT_MUTEX mMutex;

public:
  RtSharedState()
  {
    rt_mutex_create(&mMutex, "RtSharedStateMutex");
  }

  void Set(bool b)
  {
    rt_mutex_acquire_until(&mMutex, TM_INFINITE);
    mState = b;
    rt_mutex_release(&mMutex);
  }

  BOOL Get()
  {
    rt_mutex_acquire_until(&mMutex, TM_INFINITE);
    auto currentState = mState;
    rt_mutex_release(&mMutex);
    return currentState;
  }
};


RT_TASK rtSwitchStateTask;

//RT_TASK rtResistanceArrayTask;
//
//static auto rtSharedResistanceArray = std::make_shared<RtSharedResistanceArray>();
//static auto rtTaskHandler = std::make_unique<RtTaskHandler>();
static auto testingModel = testingModelClass();
static RtSharedState rtSharedState;


void GenerateSwitchStateRoutine(void*)
{
  testingModel.initialize();

  while(true)
  {
    testingModel.step();

    rtSharedState.Set(!rtSharedState.Get());

    rt_task_wait_period(NULL);

    printf("state is: %s\n", rtSharedState.Get() ? "true" : "false");
  }
}

int StartGenerateSwitchStateRoutine()
{
  int e1 = rt_task_create(&rtSwitchStateTask, "GenerateSwitchStateRoutine",
    kTaskStackSize, kMediumTaskPriority, kTaskMode);
  int e2 = rt_task_set_periodic(&rtSwitchStateTask, TM_NOW, rt_timer_ns2ticks(1000000));
  int e3 = rt_task_start(&rtSwitchStateTask, &GenerateSwitchStateRoutine, NULL);

  if(e1 | e2 | e3)
  {
    printf("Error launching StartGenerateSwitchStateRoutine(). Exiting.\n");
  }
}

void TerminationHandler(int s)
{
  printf("Caught ctrl + c signal. Closing Card and Exiting.\n");
//  PIL_ClearCard(rtTaskHandler->mCardNum);
//  PIL_CloseSpecifiedCard(rtTaskHandler->mCardNum);
//  testingModel.terminate();

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
//
//  DWORD cardNum = 3;
//  rtTaskHandler->OpenCard(cardNum);
//  rtTaskHandler->mRtSharedResistanceArray = rtSharedResistanceArray;
//  rtTaskHandler->StartSetSubunitResistanceRoutine();

  while(true) // original parent process will wait until ctrl+c signal
  {}

  return 0;
}
