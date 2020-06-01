#include <memory>

#include <RtMacro.h>
#include <RtPeakCanReceiveTask.h>

std::unique_ptr<RtPeakCanReceiveTask> rtPeakCanReceiveTask;

void TerminationHandler(int s)
{
  printf("Caught ctrl + c signal. Closing and Exiting.\n");
  rtPeakCanReceiveTask.reset();
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

  auto rtPeakCanReceiveTask = std::make_unique<RtPeakCanReceiveTask>(
    "/dev/pcanusb32", CAN_BAUD_500K, "RtPeakCanReceiveTask", RtMacro::kTaskStackSize,
    RtMacro::kMediumTaskPriority, RtMacro::kTaskMode, RtMacro::kHundredMsTaskPeriod,
    RtMacro::kCoreId6);

  rtPeakCanReceiveTask->StartRoutine();

  while(true)
  {}

  return 0;
}
