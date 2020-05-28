#include <errno.h>
#include <stdio.h>

#include <libpcan.h>

/* print out the content of a CAN message */
void print_message(const char *prompt, TPCANMsg *m)
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

int main(int argc, char **argv)
{
  HANDLE canHandle = LINUX_CAN_Open("/dev/pcanusb32", 0);
  errno = CAN_Init(canHandle, CAN_BAUD_100K, CAN_INIT_TYPE_EX);
  if(errno)
  {
    perror("peak_can_receive: error with CAN_Init()");
    return errno;
  }
  printf("Status = %i\n", CAN_Status(canHandle));


  for(;;)
  {
    TPCANRdMsg readMsg;
    if(LINUX_CAN_Read(canHandle, &readMsg))
    {
      perror("peak_can_receive: error with LINUX_CAN_READ()");
      return errno;
    }
    printf("%u.%u ", readMsg.dwTime, readMsg.wUsec);
    print_message("peak_can_receive", &(readMsg.Msg));
  }

  CAN_Close(canHandle);

  return 0;
}
