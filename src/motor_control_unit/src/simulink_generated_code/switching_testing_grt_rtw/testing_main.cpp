#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <testing.h>

#include <alchemy/mutex.h>

#include <RtGenerateStateTask.h>
#include <RtSharedState.h>
#include <RtSwitchTask.h>

static std::unique_ptr<RtSwitchTask> rtSwitchTask;
static std::unique_ptr<RtGenerateStateTask> rtGenerateStateTask;
static std::shared_ptr<RtSharedState> rtSharedState;

void TerminationHandler(int s)
{
  printf("Caught ctrl + c signal. Closing Card and Exiting.\n");

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

  rtSharedState = std::make_shared<RtSharedState>("RtSharedState");

  // TODO: specify which cpu to run task on
  rtGenerateStateTask = std::make_unique<RtGenerateStateTask>(
    "GenerateStateTask", RtTask::kStackSize, RtTask::kMediumPriority,
    RtTask::kMode, RtTime::kTenMilliseconds, RtCpu::kCore7);
  rtGenerateStateTask->mRtSharedState = rtSharedState;
  rtGenerateStateTask->StartRoutine();

  DWORD cardNum = 2;
  DWORD subunit = 1;
  DWORD bit = 1;

  rtSwitchTask = std::make_unique<RtSwitchTask>(
    "SetSubunitSwitchState", RtTask::kStackSize, RtTask::kMediumPriority,
    RtTask::kMode, RtTime::kTenMilliseconds, RtCpu::kCore6);

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
