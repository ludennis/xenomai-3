#include <PeakCanReceiveTask.h>

int main(int argc, char **argv)
{
  auto peakCanReceiveTask = PeakCanReceiveTask("/dev/pcanusb32", CAN_BAUD_500K);

  peakCanReceiveTask.Init();

  for(;;)
  {
    peakCanReceiveTask.Read();
    peakCanReceiveTask.PrintReadMessage();
  }

  return 0;
}
