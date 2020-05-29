#include <PeakCanTask.h>

PeakCanTask::PeakCanTask(
  const char *deviceName, const unsigned int baudRate)
  : mBaudRate(baudRate)
{
  mHandle = LINUX_CAN_Open(deviceName, 0);
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

/*
 * print out the content of a CAN message
 * ported from test/src/common.c
 */
void PeakCanTask::PrintMessage(const char *prompt, TPCANMsg *m)
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

PeakCanTask::~PeakCanTask()
{
  CAN_Close(mHandle);
}
