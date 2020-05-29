#include <PeakCanReceiveTask.h>

PeakCanReceiveTask::PeakCanReceiveTask(
  const char *deviceName, const unsigned int baudRate)
  : mBaudRate(baudRate)
{
  mHandle = LINUX_CAN_Open(deviceName, 0);
}

int PeakCanReceiveTask::Init()
{
  // TODO: check baudrate is one of the defined buard rates
  errno = CAN_Init(mHandle, mBaudRate, CAN_INIT_TYPE_EX);
  if(errno)
  {
    perror("PeakCanReceiveTask: Error with CAN_Init()");
    return errno;
  }
  printf("PeakCanReceiveTask: CAN_Status = %i\n", CAN_Status(mHandle));
}

int PeakCanReceiveTask::Read()
{
  if(LINUX_CAN_Read(mHandle, &mReadMessage))
  {
    perror("PeakCanReceiveTask: error with LINUX_CAN_READ()");
    return errno;
  }
}

void PeakCanReceiveTask::PrintReadMessage()
{
  printf("%u.%u ", mReadMessage.dwTime, mReadMessage.wUsec);
  PrintMessage("PeakCanReceiveTask::PrintReadMessage()", &(mReadMessage.Msg));
}

/*
 * print out the content of a CAN message
 * ported from test/src/common.c
 */
void PeakCanReceiveTask::PrintMessage(const char *prompt, TPCANMsg *m)
{
  int i;
  /* print RTR, 11 or 29, CAN-Id and datalength */
  printf("%s: %c %c 0x%08x %1d ",
      prompt,
      (m->MSGTYPE & MSGTYPE_STATUS) ? 'x' :
        ((m->MSGTYPE & MSGTYPE_RTR) ? 'r' : 'm') -
        ((m->MSGTYPE & MSGTYPE_SELFRECEIVE) ? 0x20 : 0),
      (m->MSGTYPE & MSGTYPE_STATUS) ? '-' :
        ((m->MSGTYPE & MSGTYPE_EXTENDED) ? 'e' : 's'),
       m->ID,
       m->LEN);
  /* don't print any telegram contents for remote frames */
  if (!(m->MSGTYPE & MSGTYPE_RTR))
    for (i = 0; i < m->LEN; i++)
      printf("%02x ", m->DATA[i]);
  printf("\n");
}

PeakCanReceiveTask::~PeakCanReceiveTask()
{
  CAN_Close(mHandle);
}
