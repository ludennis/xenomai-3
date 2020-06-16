#include <signal.h>
#include <chrono>
#include <thread>

#include <RtMacro.h>

#include <RtDmaDigitalInputOutputTask.h>

std::unique_ptr<RtDmaDigitalInputOutputTask> rtDmaDiTask;

void terminationHandler(int signal)
{
  // TODO: fix seg fault after termination
  rtDmaDiTask->Stop();
  rtDmaDiTask.reset();
  exit(1);
}

int main(int argc, char *argv[])
{
  struct sigaction action;
  action.sa_handler = terminationHandler;
  action.sa_flags = 0;
  sigemptyset(&action.sa_mask);
  sigaction(SIGINT , &action, NULL);

  rtDmaDiTask = std::make_unique<RtDmaDigitalInputOutputTask>(
    "rtDmaDiTask", RtMacro::kTaskStackSize, RtMacro::kHighTaskPriority, RtMacro::kTaskMode,
    RtMacro::kOneSecondTaskPeriod, RtMacro::kCoreId7);
  rtDmaDiTask->Init(argv[1], argv[2]);
  rtDmaDiTask->StartDmaChannel();
  rtDmaDiTask->EnableStreamHelper();
  rtDmaDiTask->ProgramDiSubsystem(10000u, 2u);
  rtDmaDiTask->ArmAndStartDiSubsystem();

  rtDmaDiTask->StartRoutine();

  for (;;)
  {
  }

  return 0;
}
