#include <memory>

#include <RtMacro.h>
#include <RtPeakCanTransmitTask.h>

std::unique_ptr<RtPeakCanTransmitTask> rtPeakCanTransmitTask;

void TerminationHandler(int s)
{
  printf("Caught ctrl + c signal. Closing and Exiting.\n");
  rtPeakCanTransmitTask.reset();
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

  auto rtPeakCanTransmitTask = std::make_unique<RtPeakCanTransmitTask>(
      "/dev/pcanpcifd1", CAN_BAUD_500K, "RtPeakCanTransmitTask",
      RtMacro::kTaskStackSize, RtMacro::kMediumTaskPriority, RtMacro::kTaskMode,
      RtMacro::kHundredMsTaskPeriod, RtMacro::kCoreId7);

  rtPeakCanTransmitTask->StartRoutine();

  while(true)
  {}

  return 0;
}
