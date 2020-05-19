#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include <alchemy/mutex.h>

#include <testing.h>

#include <RtGenerateResistanceArrayTask.h>
#include <RtMacro.h>
#include <RtResistanceTask.h>

RT_TASK rtResistanceArrayTask;

static auto rtSharedResistanceArray = std::make_shared<RtSharedResistanceArray>();
static std::unique_ptr<RtResistanceTask> rtResistanceTask;
static std::unique_ptr<RtGenerateResistanceArrayTask> rtGenerateResistanceArrayTask;

void TerminationHandler(int s)
{
  printf("Caught ctrl + c signal. Closing Card and Exiting.\n");
  PIL_ClearCard(rtResistanceTask->mCardNum);
  PIL_CloseSpecifiedCard(rtResistanceTask->mCardNum);
  // TODO: add task delete

  rtGenerateResistanceArrayTask->~RtGenerateResistanceArrayTask();
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

  rtGenerateResistanceArrayTask = std::make_unique<RtGenerateResistanceArrayTask>(
    "GenerateResistanceArrayRoutine", RtMacro::kTaskStackSize, RtMacro::kMediumTaskPriority,
    RtMacro::kTaskMode, RtMacro::kTenMsTaskPeriod);
  rtGenerateResistanceArrayTask->mRtSharedResistanceArray = rtSharedResistanceArray;
  rtGenerateResistanceArrayTask->StartRoutine();

  DWORD cardNum = 3;
  rtResistanceTask = std::make_unique<RtResistanceTask>(
    "SetSubunitResistanceRoutine", RtMacro::kTaskStackSize, RtMacro::kMediumTaskPriority,
    RtMacro::kTaskMode, RtMacro::kTenMsTaskPeriod);
  rtResistanceTask->OpenCard(cardNum);
  rtResistanceTask->mRtSharedResistanceArray = rtSharedResistanceArray;
  rtResistanceTask->StartRoutine();

  while(true) // original parent process will wait until ctrl+c signal
  {}

  return 0;
}
