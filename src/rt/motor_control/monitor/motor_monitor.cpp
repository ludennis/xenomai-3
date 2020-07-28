#include <stdlib.h>
#include <sys/mman.h>

#include <iostream>

#include <alchemy/heap.h>
#include <alchemy/pipe.h>
#include <alchemy/task.h>
#include <alchemy/queue.h>

#include <RtMacro.h>
#include <MessageTypes.h>

RT_HEAP rtHeap;

RT_PIPE rtPipe;

RT_QUEUE rtMotorOutputQueue;

RT_TASK rtForwardMotorOutputToPipeTask;

void ForwardMotorOutputToPipeRoutine(void*)
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
      rt_printf("[motor|monitor] Sending queue binding error\n");

    // to store the motor message from queue
    auto bytesRead =
      rt_queue_read(&rtMotorOutputQueue, blockPointer, RtQueue::kMessageSize, TM_INFINITE);

    if (bytesRead > 0)
    {
      // check message type
      int messageType;
      memcpy(&messageType, blockPointer, sizeof(int));

      if (messageType == tMotorOutputMessage)
      {
        MotorOutputMessage motorOutputMessage;

        // forward to message pipe
        auto bytesWritten =
          rt_pipe_write(&rtPipe, blockPointer, RtMessage::kMessageSize, P_NORMAL);
        rt_printf("[motor|monitor] forwarded %ld bytes to pipe\n", bytesWritten);

        // output the message
        memcpy(&motorOutputMessage, blockPointer, sizeof(MotorOutputMessage));
        rt_printf("[motor|monitor] Received MotorOutputMessage, ft_CurrentU: %f, ft_CurrentV: %f, "
          "ft_CurrentW: %f, ft_RotorRPM: %f, ft_RotorDegreeRad: %f, "
          "ft_OutputTorque: %f\n", motorOutputMessage.ft_CurrentU,
          motorOutputMessage.ft_CurrentV, motorOutputMessage.ft_CurrentW,
          motorOutputMessage.ft_RotorRPM, motorOutputMessage.ft_RotorDegreeRad,
          motorOutputMessage.ft_OutputTorque);
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

  // message pipe
  char deviceName[] = "/dev/rtp1";
  rt_pipe_create(&rtPipe, "rtPipeRtp1", 1, RtMessage::kMessageSize * 10);

  // allocate heap
  rt_heap_create(&rtHeap, "rtHeap", RtQueue::kMessageSize, H_SINGLE);
  rt_printf("[motor|monitor] Heap Created\n");

  cpu_set_t cpuSet;
  CPU_ZERO(&cpuSet);
  CPU_SET(7, &cpuSet);
  CPU_SET(8, &cpuSet);

  rt_task_create(&rtForwardMotorOutputToPipeTask, "rtForwardMotorOutputToPipeTask",
    RtTask::kStackSize, RtTask::kMediumPriority, RtTask::kMode);
  rt_task_set_periodic(&rtForwardMotorOutputToPipeTask, TM_NOW,
    rt_timer_ns2ticks(RtTime::kHundredMilliseconds));
  rt_task_set_affinity(&rtForwardMotorOutputToPipeTask, &cpuSet);
  rt_task_start(&rtForwardMotorOutputToPipeTask, ForwardMotorOutputToPipeRoutine, NULL);

  for (;;)
  {}

  return 0;
}
