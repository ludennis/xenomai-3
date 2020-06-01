#include <PeakCanReceiveTask.h>

PeakCanReceiveTask::PeakCanReceiveTask(
  const char *deviceName, const unsigned int baudRate)
  : PeakCanTask::PeakCanTask(deviceName, baudRate)
{}

int PeakCanReceiveTask::Read()
{
  if(LINUX_CAN_Read(mHandle, &mReadMessage))
  {
    perror("PeakCanReceiveTask: error with LINUX_CAN_READ()");
    return errno;
  }
}
