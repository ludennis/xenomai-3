#include <iostream>

#include <alchemy/heap.h>
#include <alchemy/pipe.h>
#include <alchemy/queue.h>
#include <alchemy/task.h>

#include <RtMacro.h>
#include <MessageTypes.h>

RT_HEAP rtDdsPipeHeap;

RT_PIPE rtDdsPipe;

RT_QUEUE rtMotorOutputQueue;

RT_TASK rtDdsPipeTask;

void DdsPipeRoutine(void*)
{
  for (;;)
  {
    void *blockPointer;
    rt_heap_alloc(&rtDdsPipeHeap, RtQueue::kMessageSize, TM_INFINITE, &blockPointer);

    auto bytesRead = rt_queue_read(
      &rtMotorOutputQueue, blockPointer, RtQueue::kMessageSize, TM_INFINITE);

    if (bytesRead)
    {
      rt_printf("Bytes Read: %d\n", bytesRead);

      int messageType;
      memcpy(&messageType, blockPointer, sizeof(int));

      if (messageType == tMotorOutputMessage)
      {
        rt_printf("Received motor output message\n");

        rt_pipe_write(&rtDdsPipe, blockPointer, RtQueue::kMessageSize, P_NORMAL);
      }
    }

    rt_task_wait_period(NULL);
  }
}

int main(int argc, char *argv[])
{
  auto outputQueueBound = rt_queue_bind(&rtMotorOutputQueue, "rtMotorOutputQueue", TM_INFINITE);
  if (outputQueueBound != 0)
    rt_printf("rt_queue_bind error\n");

  rt_heap_create(&rtDdsPipeHeap, "rtDdsPipeHeap", RtQueue::kMessageSize, H_SINGLE);

  rt_pipe_create(&rtDdsPipe, "rtDdsPipe", 1, RtQueue::kMessageSize);

  cpu_set_t cpuSet;
  CPU_ZERO(&cpuSet);
  CPU_SET(5, &cpuSet);
  CPU_SET(6, &cpuSet);
  CPU_SET(7, &cpuSet);
  CPU_SET(8, &cpuSet);
  rt_task_create(&rtDdsPipeTask, "rtDdsPipeTask", RtTask::kStackSize,
    RtTask::kLowPriority, RtTask::kMode);
  rt_task_set_periodic(&rtDdsPipeTask, TM_NOW, rt_timer_ns2ticks(RtTime::kHundredMilliseconds));
  rt_task_set_affinity(&rtDdsPipeTask, &cpuSet);
  rt_task_start(&rtDdsPipeTask, DdsPipeRoutine, NULL);

  for (;;)
  {}

  return 0;
}
