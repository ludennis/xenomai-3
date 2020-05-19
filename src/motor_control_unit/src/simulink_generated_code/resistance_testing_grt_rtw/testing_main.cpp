#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <alchemy/mutex.h>

#include <testing.h>

#include <RtGenerateResistanceArrayTask.h>
#include <RtMacro.h>
#include <RtResistanceTask.h>

// TODO: delete this
RT_TASK rtResistanceArrayTask;

static std::shared_ptr<RtSharedArray> rtSharedArray;
static std::unique_ptr<RtResistanceTask> rtResistanceTask;
static std::unique_ptr<RtGenerateResistanceArrayTask> rtGenerateResistanceArrayTask;

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

  rtSharedArray = std::make_shared<RtSharedArray>("RtSharedArray");

  rtGenerateResistanceArrayTask = std::make_unique<RtGenerateResistanceArrayTask>(
    "GenerateResistanceArrayRoutine", RtMacro::kTaskStackSize, RtMacro::kMediumTaskPriority,
    RtMacro::kTaskMode, RtMacro::kTenMsTaskPeriod);
  rtGenerateResistanceArrayTask->mRtSharedArray = rtSharedArray;
  rtGenerateResistanceArrayTask->StartRoutine();

  DWORD cardNum = 3;
  rtResistanceTask = std::make_unique<RtResistanceTask>(
    "SetSubunitResistanceRoutine", RtMacro::kTaskStackSize, RtMacro::kMediumTaskPriority,
    RtMacro::kTaskMode, RtMacro::kTenMsTaskPeriod);
  rtResistanceTask->OpenCard(cardNum);
  rtResistanceTask->mRtSharedArray = rtSharedArray;
  rtResistanceTask->StartRoutine();

  while(true) // original parent process will wait until ctrl+c signal
  {}

  return 0;
}
