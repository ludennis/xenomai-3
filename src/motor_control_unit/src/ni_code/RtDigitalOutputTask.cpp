#include <RtDigitalOutputTask.h>

RtDigitalOutputTask::RtDigitalOutputTask(
  const char *name, const int stackSize, const int priority, const int mode,
  const int period, const int coreId)
  : RtPeriodicTask(name, stackSize, priority, mode, period, coreId)
{}

int RtDigitalOutputTask::StartRoutine()
{
  int e1 = rt_task_create(&mRtTask, mName, mStackSize, mPriority, mMode);
  int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(mPeriod));
  int e3 = rt_task_set_affinity(&mRtTask, &mCpuSet);
  int e4 = rt_task_start(&mRtTask, RtDigitalOutputTask::Routine, NULL);
}

static void RtDigitalOutputTask::Routine(void*)
{
  // have the periodic task write to ports
  for(;;)
  {
    printf("Writing 0x%0X to port0.\n", outputDataPort0 & lineMaskPort0);
    dioHelper->writePresentValue(lineMaskPort0, outputDataPort0, status);

    printf("Writing 0x%0X to port1.\n", outputDataPort1 & lineMaskPort1);
    printf("Writing 0x%0X to port2.\n", outputDataPort2 & lineMaskPort2);
    pfiDioHelper->writePresentValue(lineMaskPFI, outputDataPFI, status);

    printf("\n");

    rt_task_wait_period(NULL);
  }
}

RtDigitalOutputTask::~RtDigitalOutputTask()
{}
