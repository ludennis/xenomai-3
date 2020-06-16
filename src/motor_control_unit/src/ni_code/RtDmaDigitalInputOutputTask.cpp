#include <RtDmaDigitalInputOutputTask.h>

RtDmaDigitalInputOutputTask::RtDmaDigitalInputOutputTask(
  const char *name, const int stackSize, const int priority, const int mode,
  const int period, const int coreId)
  : RtPeriodicTask(name, stackSize, priority, mode, period, coreId)
{}

void RtDmaDigitalInputOutputTask::StartRoutine()
{
  int e1 = rt_task_create(&mRtTask, mName, mStackSize, mPriority, mMode);
  int e2 = rt_task_set_periodic(&mRtTask, TM_NOW, rt_timer_ns2ticks(mPeriod));
  int e3 = rt_task_set_affinity(&mRtTask, &mCpuSet);
  int e4 = rt_task_start(&mRtTask, Routine, NULL);

  if (e1 | e2 | e3 | e4)
  {
    printf("RtDmaDigitalInputOutputTask::StartRoutine Error.\n");
    exit(-1);
  }
}

void RtDmaDigitalInputOutputTask::Routine(void*)
{
  for (;;)
  {
    dma->read(0, NULL, &bytesAvailable, allowOverwrite, &hasDataOverwritten, status);
    if (status.isFatal())
    {
      dmaError = status;
      hasDmaError = kTrue;
      return;
    }
    else if (bytesAvailable >= readSizeInBytes)
    {
      dma->read(readSizeInBytes, &rawData[0], &bytesAvailable, allowOverwrite,
        &hasDataOverwritten, status);
      if (status.isNotFatal())
      {
        nNISTC3::nDIODataHelper::printData(rawData, readSizeInBytes, sampleSizeInBytes);
        bytesRead += readSizeInBytes;

        if (!allowOverwrite)
        {
          streamHelper->modifyTransferSize(readSizeInBytes, status);
        }
      }
      else
      {
        dmaError = status;
        hasDmaError = kTrue;
      }
    }

    device->DI.DI_Timer.Status_1_Register.refresh(&status);
    scanOverrun = device->DI.DI_Timer.Status_1_Register.getOverrun_St(&status);
    fifoOverflow = device->DI.DI_Timer.Status_1_Register.getOverflow_St(&status);

    if (scanOverrun || fifoOverflow)
    {
      hasDiError = kTrue;
    }

    rt_task_wait_period(NULL);
  }
}

RtDmaDigitalInputOutputTask::~RtDmaDigitalInputOutputTask()
{}
