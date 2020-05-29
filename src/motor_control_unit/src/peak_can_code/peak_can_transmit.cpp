#include <PeakCanTransmitTask.h>

int main(int argc, char **argv)
{
  auto peakCanTransmitTask = PeakCanTransmitTask("/dev/pcanpcifd1", CAN_BAUD_500K);
  peakCanTransmitTask.Init();

  TPCANMsg writeMsg;
  writeMsg.ID = 0x123;
  writeMsg.MSGTYPE = MSGTYPE_STANDARD;
  writeMsg.LEN = 3;
  writeMsg.DATA[0] = 0x1f;
  writeMsg.DATA[1] = 0x2a;
  writeMsg.DATA[2] = 0x3e;

  peakCanTransmitTask.Write(writeMsg);

  return 0;
}
