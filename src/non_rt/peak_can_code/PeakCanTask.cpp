#include <PeakCanTask.h>

HANDLE PeakCanTask::mHandle;

PeakCanTask::PeakCanTask(
  const char *deviceName, const unsigned int baudRate)
{
  mHandle = LINUX_CAN_Open(deviceName, 0);

  mBaudRate = (baudRate == 1000) ? 0x0014 :
              (baudRate == 500) ? 0x001C :
              (baudRate == 250) ? 0x011C :
              (baudRate == 125) ? 0x031C :
              (baudRate == 100) ? 0x432F :
              (baudRate == 50) ? 0x472F :
              (baudRate == 20) ? 0x532F :
              (baudRate == 10) ? 0x672F :
              (baudRate == 5) ? 0x7F7F : 0;

  if(mBaudRate == 0)
  {
    printf("BaudRate not good, please specify Baud Rate to be one of the following: "
      "%dk|%dk|%dk|%dk|%dk|%dk|%dk|%dk|%dk\n", 1000, 500, 250, 125, 100, 50, 20, 10, 5);
    exit(-1);
  }

  Init();
}

int PeakCanTask::Init()
{
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
