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
    rt_heap_alloc(&rtDdsPipeHeap, RtMessage::kMessageSize, TM_INFINITE, &blockPointer);

    auto bytesRead = rt_queue_read(
      &rtMotorOutputQueue, blockPointer, RtMessage::kMessageSize, TM_INFINITE);

    if (bytesRead)
    {
      rt_printf("Bytes Read: %d\n", bytesRead);

      int messageType;
      memcpy(&messageType, blockPointer, sizeof(int));

      if (messageType == tMotorMessage)
      {
        rt_printf("Received motor output message\n");
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

  rt_heap_create(&rtDdsPipeHeap, "rtDdsPipeHeap", RtMessage::kMessageSize, H_SINGLE);

  rt_task_create(&rtDdsPipeTask, "rtDdsPipeTask", RtTask::kStackSize,
    RtTask::kLowPriority, RtTask::kMode);
  rt_task_set_periodic(&rtDdsPipeTask, TM_NOW, rt_timer_ns2ticks(RtTime::kHundredMilliseconds));
  rt_task_start(&rtDdsPipeTask, DdsPipeRoutine, NULL);

  for (;;)
  {}

  return 0;
}
