#include <RtDigitalInputTask.h>

RtDigitalInputTask::RtDigitalInputTask(
  const char *name, const int stackSize, const int priority, const int mode,
  const int period, const int coreId)
  : RtPeriodicTask::RtPeriodicTask(name, stackSize, priority, mode, period, coreId)
{}

int RtDigitalInputTask::StartRoutine()
{
  int e1 = rt_task_create(&mRtTask, mName, mStackSize, mPriority, mMode);
  int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(mPeriod));
  int e3 = rt_task_set_affinity(&mRtTask, &mCpuSet);
  int e4 = rt_task_start(&mRtTask, RtDigitalInputTask::Routine, NULL);
}

void RtDigitalInputTask::Routine(void*)
{
  for(;;)
  {
    dioHelper->readPresentValue(lineMaskPort0, inputDataPort0, status);
    pfiDioHelper->readPresentValue(lineMaskPFI, inputDataPFI, status);

    printf("Read 0x%0X from port0.\n", inputDataPort0);
    printf("Read 0x%0X from port1.\n", inputDataPFI & lineMaskPort1);
    printf("Read 0x%0X from port2.\n", (inputDataPFI >> port1Length) & lineMaskPort2);
    printf("\n");

    rt_task_wait_period(NULL);
  }
}

RtDigitalInputTask::~RtDigitalInputTask()
{}
