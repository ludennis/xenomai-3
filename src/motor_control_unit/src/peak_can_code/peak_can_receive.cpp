#include <RtMacro.h>
#include <RtPeakCanReceiveTask.h>

int main(int argc, char **argv)
{
  auto rtPeakCanReceiveTask = RtPeakCanReceiveTask(
    "/dev/pcanusb32", CAN_BAUD_500K, "RtPeakCanReceiveTask", RtMacro::kTaskStackSize,
    RtMacro::kMediumTaskPriority, RtMacro::kTaskMode, RtMacro::kTenMsTaskPeriod,
    RtMacro::kCoreId6);

  rtPeakCanReceiveTask.StartRoutine();

  while(true)
  {}

  return 0;
}
