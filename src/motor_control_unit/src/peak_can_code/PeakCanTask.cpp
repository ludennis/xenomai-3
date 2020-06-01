#include <PeakCanTask.h>

HANDLE PeakCanTask::mHandle;

PeakCanTask::PeakCanTask(
  const char *deviceName, const unsigned int baudRate)
  : mBaudRate(baudRate)
{
  mHandle = LINUX_CAN_Open(deviceName, 0);
  Init();
}

int PeakCanTask::Init()
{
  // TODO: check if baudrate is one of the defined baud rates
  errno = CAN_Init(mHandle, mBaudRate, CAN_INIT_TYPE_EX);
  if(errno)
  {
    perror("PeakCanTask: Error with CAN_Init()");
    return errno;
  }
  printf("PeakCanTask: CAN_Status = %i\n", CAN_Status(mHandle));
}

PeakCanTask::~PeakCanTask()
{
  CAN_Close(mHandle);
}
