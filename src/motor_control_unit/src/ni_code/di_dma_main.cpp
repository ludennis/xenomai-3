#include <signal.h>
#include <chrono>
#include <thread>

#include <RtMacro.h>

#include <DmaDigitalInputOutputTask.h>

std::unique_ptr<DmaDigitalInputOutputTask> dmaDiTask;

void terminationHandler(int signal)
{
  dmaDiTask->Stop();
  dmaDiTask.reset();
  exit(1);
}

int main(int argc, char *argv[])
{
  struct sigaction action;
  action.sa_handler = terminationHandler;
  action.sa_flags = 0;
  sigemptyset(&action.sa_mask);
  sigaction(SIGINT , &action, NULL);

  dmaDiTask = std::make_unique<DmaDigitalInputOutputTask>(
    "DmaDiTask", RtMacro::kTaskStackSize, RtMacro::kHighTaskPriority, RtMacro::kTaskMode,
    RtMacro::kOneSecondTaskPeriod, RtMacro::kCoreId7);
  dmaDiTask->Init(argv[1], argv[2]);
  dmaDiTask->StartDmaChannel();
  dmaDiTask->EnableStreamHelper();
  dmaDiTask->ProgramDiSubsystem(10000u, 2u);
  dmaDiTask->ArmAndStartDiSubsystem();

  dmaDiTask->StartRoutine();

  for (;;)
  {
  }

  return 0;
}
