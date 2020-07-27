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
  if(argc < 3)
  {
    printf("Usage: peak_can_receive [device name] [baud rate (Kbits/s)]\n");
    return -1;
  }
  const char *deviceName = argv[1];
  const unsigned int baudRate = atol(argv[2]);

  // ctrl + c signal handler
  struct sigaction signalHandler;
  signalHandler.sa_handler = TerminationHandler;
  sigemptyset(&signalHandler.sa_mask);
  signalHandler.sa_flags = 0;
  sigaction(SIGINT, &signalHandler, NULL);

  auto rtPeakCanReceiveTask = std::make_unique<RtPeakCanReceiveTask>(
    deviceName, baudRate, "RtPeakCanReceiveTask", RtTask::kStackSize,
    RtTask::kMediumPriority, RtTask::kMode, RtTime::kTenMilliseconds,
    RtCpu::kCore6);

  rtPeakCanReceiveTask->StartRoutine();

  while(true)
  {}

  return 0;
}
