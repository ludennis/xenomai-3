#include <PeakCanTransmitTask.h>

PeakCanTransmitTask::PeakCanTransmitTask(
  const char *deviceName, const unsigned int baudRate)
  : mBaudRate(baudRate)
{
  mHandle = LINUX_CAN_Open(deviceName, 0);
}

int PeakCanTransmitTask::Init()
{
  errno = CAN_Init(mHandle, mBaudRate, CAN_INIT_TYPE_EX);
  if(errno)
  {
    perror("PeakCanTransmitTask: Error with CAN_Init()");
    return errno;
  }
  printf("PeakCanTransmitTask: CAN_Status = %i\n", CAN_Status(mHandle));
}

int PeakCanTransmitTask::Write(TPCANMsg &writeMessage)
{
  CAN_Write(mHandle, &writeMessage);
}

PeakCanTransmitTask::~PeakCanTransmitTask()
{
  CAN_Close(mHandle);
}
