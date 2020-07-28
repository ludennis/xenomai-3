#include <sys/mman.h>

#include <iostream>
#include <limits>
#include <signal.h>

#include <alchemy/heap.h>
#include <alchemy/pipe.h>
#include <alchemy/queue.h>
#include <alchemy/task.h>

#include <MessageTypes.h>
#include <RtMacro.h>

RT_HEAP rtHeap;
RT_PIPE rtPipe;
RT_QUEUE rtMotorInputQueue;
RT_QUEUE rtMotorOutputQueue;
RT_TASK rtForwardMotorInputFromPipeTask;
RT_TASK rtReceiveMotorOutputTask;

void ForwardMotorInputFromPipeRoutine(void*)
{
  for (;;)
  {
    void *pipeBufferRead = malloc(RtMessage::kMessageSize);

    auto retval = rt_pipe_read(
      &rtPipe, pipeBufferRead, RtMessage::kMessageSize, TM_INFINITE);

    if (retval <= 0)
    {
      rt_printf("[motor|controller] rt_pipe_read error: %s\n", strerror(-retval));
    }
    else
    {
      void *queueBufferSend = rt_queue_alloc(&rtMotorInputQueue, RtQueue::kMessageSize);
      memcpy(queueBufferSend, pipeBufferRead, RtQueue::kMessageSize);
      retval = rt_queue_send(
        &rtMotorInputQueue, queueBufferSend, RtQueue::kMessageSize, Q_NORMAL);

      // TODO: check if it's motor input message and its validity

      if (retval < 0)
      {
        rt_printf("[motor|controller] rt_queue_send error: %s\n", strerror(retval));
      }
      else
      {
        rt_printf("[motor|controller] Forwarded Motor Input Message (size = %ld bytes)\n", retval);
      }
    }

    free(pipeBufferRead);
  }

}

void ReceiveMotorOutputRoutine(void*)
{
  for (;;)
  {
    void *blockPointer;
    rt_heap_alloc(&rtHeap, RtQueue::kMessageSize, TM_INFINITE, &blockPointer);

    rt_queue_bind(&rtMotorOutputQueue, "rtMotorOutputQueue", TM_INFINITE);
    auto bytesRead = rt_queue_read(
      &rtMotorOutputQueue, blockPointer, RtQueue::kMessageSize, TM_INFINITE);

    if (bytesRead)
    {
      int messageType;
      memcpy(&messageType, blockPointer, sizeof(int));

      if (messageType == tMotorOutputMessage)
      {
        auto motorOutputMessage = MotorOutputMessage{};
        memcpy(&motorOutputMessage, blockPointer, sizeof(MotorOutputMessage));

        #ifdef MOTOR_CONTROL_DEBUG
        rt_printf("[motor|controller] Received MotorOutputMessage, ft_CurrentU: %f, ft_CurrentV: %f, "
          "ft_CurrentW: %f, ft_RotorRPM: %f, ft_RotorDegreeRad: %f, "
          "ft_OutputTorque: %f\n", motorOutputMessage.ft_CurrentU, motorOutputMessage.ft_CurrentV,
          motorOutputMessage.ft_CurrentW, motorOutputMessage.ft_RotorRPM,
          motorOutputMessage.ft_RotorDegreeRad, motorOutputMessage.ft_OutputTorque);
        #endif
      }
    }

    rt_heap_free(&rtHeap, blockPointer);
  }
}

void TerminationHandler(int s)
{
  printf("Controller Exiting\n");

  rt_task_suspend(&rtReceiveMotorOutputTask);
  rt_task_delete(&rtReceiveMotorOutputTask);
  rt_printf("[motor|controller] rtReceiveMotorOutputTask finished\n");
  rt_task_suspend(&rtForwardMotorInputFromPipeTask);
  rt_task_delete(&rtForwardMotorInputFromPipeTask);
  rt_printf("[motor|controller] rtForwardMotorInputFromPipeTask finished\n");

  rt_heap_delete(&rtHeap);
  rt_pipe_delete(&rtPipe);
  rt_queue_delete(&rtMotorInputQueue);
}

int main(int argc, char *argv[])
{
  struct sigaction signalHandler;
  signalHandler.sa_handler = TerminationHandler;
  sigemptyset(&signalHandler.sa_mask);
  signalHandler.sa_flags = 0;
  sigaction(SIGINT, &signalHandler, NULL);

  printf("Connecting to motor\n");

  mlockall(MCL_CURRENT | MCL_FUTURE);

  cpu_set_t cpuSet;

  // task for receiving motor output
  rt_heap_create(&rtHeap, "rtControllerHeap", RtQueue::kMessageSize, H_SINGLE);
  rt_task_create(&rtReceiveMotorOutputTask, "rtControlReceiveMotorOutputTask",
    RtTask::kStackSize, RtTask::kHighPriority, T_JOINABLE);

  CPU_ZERO(&cpuSet);
  CPU_SET(7, &cpuSet);
  rt_task_set_affinity(&rtReceiveMotorOutputTask, &cpuSet);

  rt_task_start(&rtReceiveMotorOutputTask, ReceiveMotorOutputRoutine, NULL);

  // task for forwarding motor input from message pipe
  rt_queue_create(&rtMotorInputQueue, "rtMotorInputQueue",
    RtQueue::kMessageSize * RtQueue::kQueueLimit, RtQueue::kQueueLimit, Q_FIFO);
  rt_pipe_create(&rtPipe, "rtPipeRtp0", 0, RtMessage::kMessageSize * 10);
  rt_task_create(&rtForwardMotorInputFromPipeTask, "rtForwardMotorInputFromPipeTask",
    RtTask::kStackSize, RtTask::kMediumPriority, T_JOINABLE);

  CPU_ZERO(&cpuSet);
  CPU_SET(6, &cpuSet);
  rt_task_set_affinity(&rtForwardMotorInputFromPipeTask, &cpuSet);

  rt_task_start(&rtForwardMotorInputFromPipeTask, ForwardMotorInputFromPipeRoutine, NULL);

  rt_task_join(&rtReceiveMotorOutputTask);
  rt_task_join(&rtForwardMotorInputFromPipeTask);

  return 0;
}
