#include <stdio.h>

#include <libpcan.h>

int main(int argc, char **argv)
{
  HANDLE canHandle = LINUX_CAN_Open("/dev/pcanpcifd1", 0);
  CAN_Init(canHandle, CAN_BAUD_100K, CAN_INIT_TYPE_EX);
  printf("Status = %i\n", CAN_Status(canHandle));

  TPCANMsg writeMsg;
  writeMsg.ID = 0x123;
  writeMsg.MSGTYPE = MSGTYPE_STANDARD;
  writeMsg.LEN = 3;
  writeMsg.DATA[0] = 0x1;
  writeMsg.DATA[1] = 0x2;
  writeMsg.DATA[2] = 0x3;

  CAN_Write(canHandle, &writeMsg);

  CAN_Close(canHandle);

  return 0;
}
