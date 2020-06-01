#include <RtMacro.h>
#include <RtPeakCanTransmitTask.h>

int main(int argc, char **argv)
{
  auto rtPeakCanTransmitTask =
    RtPeakCanTransmitTask("/dev/pcanpcifd1", CAN_BAUD_500K, "RtPeakCanTransmitTask",
      RtMacro::kTaskStackSize, RtMacro::kMediumTaskPriority, RtMacro::kTaskMode,
      RtMacro::kHundredMsTaskPeriod, RtMacro::kCoreId7);

  rtPeakCanTransmitTask.StartRoutine();

  while(true)
  {}

  return 0;
}
