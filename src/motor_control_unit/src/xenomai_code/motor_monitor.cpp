#include <stdlib.h>
#include <sys/mman.h>

#include <iostream>

#include <alchemy/heap.h>
#include <alchemy/task.h>
#include <alchemy/queue.h>

#include <RtMacro.h>
#include <MessageTypes.h>

RT_HEAP rtHeap;

RT_QUEUE rtQueue;

RT_TASK rtReceiveMotorMessageTask;

void ReceiveMotorMessage(void*)
{
  // alloc heap block
  void *blockPointer;
  rt_heap_alloc(&rtHeap, RtMessage::kMessageSize, TM_INFINITE, &blockPointer);

  for (;;)
  {
    // to store the motor message from queue
    auto bytesRead =
      rt_queue_read(&rtQueue, blockPointer, RtMessage::kMessageSize, TM_INFINITE);

    if (bytesRead > 0)
    {
      rt_printf("bytes read: %d\n", bytesRead);
      // check message type
      int *messageType = (int*) malloc(sizeof(int));
      memcpy(messageType, blockPointer, sizeof(int));

//      rt_printf("*messageType = %d, blockPointer = %d\n", *messageType, blockPointer);

      if (*messageType == tMotorMessage)
      {
        rt_printf("Received MotorMessage\n");

        // do whatever with the message
      }

      free(messageType);
    }

    rt_task_wait_period(NULL);
  }
}

// termination
void TerminationHandler(int signal)
{
  printf("Termination signal received. Exiting\n");
  rt_queue_delete(&rtQueue);
  printf("Deleted rt queue\n");
  rt_heap_delete(&rtHeap);
  printf("Deleted rt heap\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  // sigaction
  struct sigaction action;
  action.sa_handler = TerminationHandler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, NULL);

  mlockall(MCL_CURRENT|MCL_FUTURE);

  // allocate queue
  rt_queue_create(&rtQueue, "rtQueue", RtMessage::kMessageSize,
    RtQueue::kQueueLimit, Q_NORMAL);
  rt_printf("Queue Created\n");

  // allocate heap
  rt_heap_create(&rtHeap, "rtHeap", RtMessage::kMessageSize, H_SINGLE);
  rt_printf("Heap Created\n");

  rt_task_create(&rtReceiveMotorMessageTask, "rtReceiveMotorMessageTask",
    RtTask::kStackSize, RtTask::kMediumPriority, RtTask::kMode);
  rt_task_set_periodic(&rtReceiveMotorMessageTask, TM_NOW,
    rt_timer_ns2ticks(RtTime::kHundredMilliseconds));
  rt_task_start(&rtReceiveMotorMessageTask, ReceiveMotorMessage, NULL);

  for (;;)
  {}

  return 0;
}
