#include <RtDigitalOutputTask.h>

RtDigitalOutputTask::RtDigitalOutputTask(
  const char *name, const int stackSize, const int priority, const int mode,
  const int period, const int coreId)
  : RtPeriodicTask(name, stackSize, priority, mode, period, coreId)
{
}

int RtDigitalOutputTask::StartRoutine()
{
}

static void RtDigitalOutputTask::Routine(void*)
{
}
