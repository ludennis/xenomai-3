#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <testing.h>

#include <alchemy/mutex.h>

#include <RtGenerateStateTask.h>
#include <RtSharedState.h>
#include <RtSwitchTask.h>

static auto rtSwitchTask = std::make_unique<RtSwitchTask>();
static auto rtGenerateStateTask = std::make_unique<RtGenerateStateTask>();
static auto rtSharedState = std::make_shared<RtSharedState>();

void TerminationHandler(int s)
{
  printf("Caught ctrl + c signal. Closing Card and Exiting.\n");

  // TODO: make close card a destructor for PxiCardTask
  PIL_ClearCard(rtSwitchTask->mCardNum);
  PIL_CloseSpecifiedCard(rtSwitchTask->mCardNum);

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

  rtGenerateStateTask = std::make_unique<RtGenerateStateTask>(
    "GenerateStateTask", RtMacro::kTaskStackSize, RtMacro::kHighTaskPriority,
    RtMacro::kTaskMode, RtMacro::kTenMsTaskPeriod);
  rtGenerateStateTask->mRtSharedState = rtSharedState;
  rtGenerateStateTask->StartRoutine();

  DWORD cardNum = 2;
  DWORD subunit = 1;
  DWORD bit = 1;

  rtSwitchTask = std::make_unique<RtSwitchTask>(
    "SetSubunitSwitchState", RtMacro::kTaskStackSize, RtMacro::kMediumTaskPriority,
    RtMacro::kTaskMode, RtMacro::kTenMsTaskPeriod);

  rtSwitchTask->OpenCard(cardNum);
  rtSwitchTask->mSubunit = subunit;
  rtSwitchTask->mBit = bit;
  rtSwitchTask->mRtSharedState = rtSharedState;
  rtSwitchTask->mOneSecondTimer = rt_timer_read();
  rtSwitchTask->StartRoutine();

  while(true) // original parent process will wait until ctrl+c signal
  {}

  return 0;
}
