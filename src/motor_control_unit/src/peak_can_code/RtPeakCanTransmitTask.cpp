#include <RtPeakCanTransmitTask.h>

RtPeakCanTransmitTask::RtPeakCanTransmitTask(
  const char *deviceName, const unsigned int baudRate,
  const char *name, const int stackSize, const int priority, const int mode,
  const int period, const int coreId)
  : PeakCanTransmitTask::PeakCanTransmitTask(deviceName, baudRate)
  , RtPeriodicTask::RtPeriodicTask(name, stackSize, priority, mode, period, coreId)
{}

int RtPeakCanTransmitTask::StartRoutine()
{
  mlockall(MCL_CURRENT|MCL_FUTURE);

  int e1 = rt_task_create(&mRtTask, mName, mStackSize, mPriority, mMode);
  int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(mPeriod));
  int e3 = (mCoreId > 0) ? rt_task_set_affinity(&mRtTask, &mCpuSet) : 0;
  int e4 = rt_task_start(&mRtTask, &Routine, NULL);

  if(e1 | e2 | e3 | e4)
  {
    printf("Error with RtPeakCanTransmitTask::StartRoutine(). Exiting.\n");
    exit(-1);
  }
}

void RtPeakCanTransmitTask::Routine(void*)
{
  TPCANMsg writeMessage;
  writeMessage.ID = 0x123;
  writeMessage.MSGTYPE = MSGTYPE_STANDARD;
  writeMessage.LEN = 3;
  writeMessage.DATA[0] = 0x1f;
  writeMessage.DATA[1] = 0x2a;
  writeMessage.DATA[2] = 0x3e;

  while(true)
  {
    CAN_Write(mHandle, &writeMessage);
    rt_task_wait_period(NULL);
  }
}

RtPeakCanTransmitTask::~RtPeakCanTransmitTask()
{}
