#include <RtPeakCanReceiveTask.h>

namespace {

/*
 * print out the content of a CAN message
 * ported from test/src/common.c
 */
void PrintMessage(const char *prompt, TPCANMsg *m)
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

} // namespace

RtPeakCanReceiveTask::RtPeakCanReceiveTask(
  const char *deviceName, const unsigned int baudRate, const char *name,
  const int stackSize, const int priority, const int mode, const int period,
  const int coreId)
  : PeakCanReceiveTask(deviceName, baudRate)
  , RtPeriodicTask(name, stackSize, priority, mode, period, coreId)
{}

int RtPeakCanReceiveTask::StartRoutine()
{
  int e1 = rt_task_create(&mRtTask, mName, mStackSize, mPriority, mMode);
  int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(mPeriod));
  int e3 = (mCoreId > 0) ? rt_task_set_affinity(&mRtTask, &mCpuSet) : 0;
  int e4 = rt_task_start(&mRtTask, &RtPeakCanReceiveTask::Routine, NULL);

  if(e1 | e2 | e3 | e4)
  {
    printf("Error with RtPeakCanReceiveTask::StartRoutine(). Exiting.\n");
    exit(-1);
  }
}

void RtPeakCanReceiveTask::Routine(void*)
{
  TPCANRdMsg readMessage;

  while(true)
  {
    // read
    if(LINUX_CAN_Read(mHandle, &readMessage))
    {
      perror("RtPeakCanReceiveTask: error with LINUX_CAN_READ()");
      exit(-1);
    }

    // print
    printf("%u.%u ", readMessage.dwTime, readMessage.wUsec);
    PrintMessage(
      "RtPeakCanReceiveTask::PrintReadMessage()", &(readMessage.Msg));

    rt_task_wait_period(NULL);
  }
}

RtPeakCanReceiveTask::~RtPeakCanReceiveTask()
{}
