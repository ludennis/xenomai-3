#include <stdlib.h>
#include <sys/mman.h>

#include <iostream>

#include <alchemy/heap.h>
#include <alchemy/task.h>
#include <alchemy/queue.h>

#include <RtMacro.h>
#include <MessageTypes.h>

RT_HEAP rtHeap;

RT_QUEUE rtMotorOutputQueue;

RT_TASK rtReceiveMotorMessageTask;

void ReceiveMotorMessage(void*)
{
  // alloc heap block
  void *blockPointer;
  rt_heap_alloc(&rtHeap, RtQueue::kMessageSize, TM_INFINITE, &blockPointer);

  for (;;)
  {
    // find sending queue before reading
    auto sendingQueueBound = rt_queue_bind(
      &rtMotorOutputQueue, "rtMotorOutputQueue", TM_INFINITE);
    if (sendingQueueBound != 0)
      rt_printf("Sending queue binding error\n");

    // to store the motor message from queue
    auto bytesRead =
      rt_queue_read(&rtMotorOutputQueue, blockPointer, RtQueue::kMessageSize, TM_INFINITE);

    if (bytesRead > 0)
    {
      // check message type
      int messageType;
      memcpy(&messageType, blockPointer, sizeof(int));

      if (messageType == tMotorMessage)
      {
        MotorMessage *motorMessage = (MotorMessage*) malloc(sizeof(MotorMessage));
        memcpy(motorMessage, blockPointer, sizeof(MotorMessage));

        rt_printf("Received MotorMessage, ft_CurrentU: %f, ft_CurrentV: %f, "
          "ft_CurrentW: %f, ft_RotorRPM: %f, ft_RotorDegreeRad: %f, "
          "ft_OutputTorque: %f\n", motorMessage->ft_CurrentU, motorMessage->ft_CurrentV,
          motorMessage->ft_CurrentW, motorMessage->ft_RotorRPM,
          motorMessage->ft_RotorDegreeRad, motorMessage->ft_OutputTorque);
      }
    }

    rt_heap_free(&rtHeap, blockPointer);

    rt_task_wait_period(NULL);
  }
}

// termination
void TerminationHandler(int signal)
{
  printf("Termination signal received. Exiting\n");
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

  // allocate heap
  rt_heap_create(&rtHeap, "rtHeap", RtQueue::kMessageSize, H_SINGLE);
  rt_printf("Heap Created\n");

  cpu_set_t cpuSet;
  CPU_ZERO(&cpuSet);
  CPU_SET(5, &cpuSet);
  CPU_SET(6, &cpuSet);
  CPU_SET(7, &cpuSet);
  CPU_SET(8, &cpuSet);

  rt_task_create(&rtReceiveMotorMessageTask, "rtReceiveMotorMessageTask",
    RtTask::kStackSize, RtTask::kMediumPriority, RtTask::kMode);
  rt_task_set_periodic(&rtReceiveMotorMessageTask, TM_NOW,
    rt_timer_ns2ticks(RtTime::kHundredMilliseconds));
  rt_task_set_affinity(&rtReceiveMotorMessageTask, &cpuSet);
  rt_task_start(&rtReceiveMotorMessageTask, ReceiveMotorMessage, NULL);

  for (;;)
  {}

  return 0;
}
