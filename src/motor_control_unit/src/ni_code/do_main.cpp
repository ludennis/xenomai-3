#include <signal.h>

#include <RtDigitalOutputTask.h>

#include <RtMacro.h>

std::unique_ptr<RtDigitalOutputTask> rtDoTask;

void TerminationHandler(int signal)
{
  printf("Caught ctrl + c signal. Exiting.\n");
  rtDoTask->~RtDigitalOutputTask();
  exit(1);
}

// TODO: make this a RT_xenomai periodic Task
int main(int argc, char* argv[])
{
  struct sigaction handler;
  handler.sa_handler = TerminationHandler;
  sigemptyset(&handler.sa_mask);
  handler.sa_flags = 0;
  sigaction(SIGINT, &handler, NULL);

  if (argc <= 2)
  {
    printf("Usage: do_main [PCI Bus Number] [PCI Device Number]\n");
    return -1;
  }

  rtDoTask = std::make_unique<RtDigitalOutputTask>(
    "RtDigitalOutputTask", RtMacro::kTaskStackSize, RtMacro::kMediumTaskPriority,
    RtMacro::kTaskMode, RtMacro::kOneMsTaskPeriod, RtMacro::kCoreId7);

  rtDoTask->lineMaskPort0 = 0xFFFFFFFF; //use all lines on port 0
  rtDoTask->lineMaskPort1 = 0xFF; //use all lines on port 1
  rtDoTask->lineMaskPort2 = 0xFF; //use all lines on port 2
  rtDoTask->triStateOnExit = kTrue;

  rtDoTask->outputDataPort0 = 0x5A5A5A5A; // value to write to port0
  rtDoTask->outputDataPort1 = 0x99; // value to write to port1
  rtDoTask->outputDataPort2 = 0x66; // value to write to port2

  rtDoTask->port0Length = 0;
  rtDoTask->port1Length = 8;

  rtDoTask->lineMaskPFI =
    (rtDoTask->lineMaskPort2 << rtDoTask->port1Length) | rtDoTask->lineMaskPort1;
  rtDoTask->outputDataPFI =
    (rtDoTask->outputDataPort2 << rtDoTask->port1Length) | rtDoTask->outputDataPort1;
  rtDoTask->inputDataPort0 = 0x0; // value of lines on port 0
  rtDoTask->inputDataPFI = 0x0; //value of lines on port1:2 (PFI0..15)

  // TODO: fix seg fault after ctrl + c termination
  // possible cause would be that dioHelper/pfiDioHelper still writing when we exit
  rtDoTask->Init(argv[1], argv[2]);
  rtDoTask->StartRoutine();

  for(;;){}

  return 0;
}
