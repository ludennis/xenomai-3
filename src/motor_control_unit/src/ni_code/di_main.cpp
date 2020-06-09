#include <signal.h>
#include <stdlib.h>

#include <memory>

#include <RtMacro.h>

#include <RtDigitalInputTask.h>

std::unique_ptr<RtDigitalInputTask> diTask;

void TerminationHandler(int s)
{
  printf("Caught ctrl + c signal. Exiting.\n");
  diTask.reset();
  exit(1);
}

int main(int argc, char *argv[])
{
  struct sigaction handler;
  handler.sa_handler = TerminationHandler;
  sigemptyset(&handler.sa_mask);
  handler.sa_flags = 0;
  sigaction(SIGINT, &handler, NULL);

  if (argc <= 2)
  {
    printf("Usage: di_main [Bus Number] [Device Number]\n");
    return -1;
  }

  diTask = std::make_unique<RtDigitalInputTask>(
    "RtDigitalInputTask", RtMacro::kTaskStackSize, RtMacro::kMediumTaskPriority,
    RtMacro::kTaskMode, RtMacro::kOneSecondTaskPeriod, RtMacro::kCoreId6);

  diTask->lineMaskPort0 = 0xFFFFFFFF;
  diTask->lineMaskPort1 = 0xFF;
  diTask->lineMaskPort2 = 0xFF;
  diTask->triStateOnExit = kTrue;

  diTask->port0Length = 0;
  diTask->port1Length = 0;

  diTask->lineMaskPFI =
    (diTask->lineMaskPort2 << diTask->port1Length) | diTask->lineMaskPort1;
  diTask->inputDataPort0 = 0x0;
  diTask->inputDataPFI = 0x0;

  diTask->Init(argv[1], argv[2]);

  diTask->StartRoutine();

  for(;;){}

  return 0;
}
