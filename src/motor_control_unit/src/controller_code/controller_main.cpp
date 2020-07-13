#include <sys/mman.h>

#include <limits>
#include <signal.h>
#include <iostream>

#include <alchemy/task.h>
#include <alchemy/heap.h>
#include <alchemy/pipe.h>
#include <alchemy/queue.h>

#include <RtMacro.h>
#include <MessageTypes.h>

RT_TASK rtMotorReceiveStepTask;
RT_TASK rtSendMotorStepTask;
RT_TASK rtReceiveMotorOutputTask;
RT_TASK rtReceiveMotorInputFromPipeTask;

RT_TASK_MCB rtSendMessage;
RT_TASK_MCB rtReceiveMessage;

RTIME rtTimerBegin;
RTIME rtTimerEnd;
RTIME rtTimerOneSecond;
RTIME rtTimerFiveSeconds;

RT_HEAP rtHeap;

RT_PIPE rtPipe;

RT_QUEUE rtMotorOutputQueue;

unsigned int numberOfMessages{0u};
unsigned int numberOfMessagesRealTime{0u};
double totalRoundTripTime;
RTIME minRoundTripTime = std::numeric_limits<RTIME>::max();
RTIME maxRoundTripTime = std::numeric_limits<RTIME>::min();

void ReceiveMotorInputFromPipeRoutine(void*)
{
  void *buffer = malloc(RtMessage::kMessageSize);

  for (;;)
  {
    auto bytesRead = rt_pipe_read(&rtPipe, buffer, RtMessage::kMessageSize, TM_INFINITE);

    if (bytesRead <= 0)
    {
      rt_printf("rt_pipe_read error: %s\n");
    }
    else
    {
      rt_printf("Read %ld bytes\n", bytesRead);
    }
  }

  free(buffer);
}

void SendMotorInputRoutine(void*)
{
  void *buffer = rt_queue_alloc(&rtMotorInputQueue, RtQueue::kMessageSize);
  MotorInputMessage motorInputMessage = MotorInputMessage{
    1, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
  memcpy(buffer, &motorInputMessage, RtQueue::kMessageSize);
  rt_queue_send(&rtMotorInputQueue, buffer, RtQueue::kMessageSize, Q_NORMAL);
}

void ReceiveMotorOutputRoutine(void*)
{
  void *blockPointer;
  rt_heap_alloc(&rtHeap, RtQueue::kMessageSize, TM_INFINITE, &blockPointer);

  for (;;)
  {
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

        rt_printf("Received MotorOutputMessage, ft_CurrentU: %f, ft_CurrentV: %f, "
          "ft_CurrentW: %f, ft_RotorRPM: %f, ft_RotorDegreeRad: %f, "
          "ft_OutputTorque: %f\n", motorOutputMessage.ft_CurrentU, motorOutputMessage.ft_CurrentV,
          motorOutputMessage.ft_CurrentW, motorOutputMessage.ft_RotorRPM,
          motorOutputMessage.ft_RotorDegreeRad, motorOutputMessage.ft_OutputTorque);
      }
    }
    rt_heap_free(&rtHeap, blockPointer);
  }
}

void TerminationHandler(int s)
{
  printf("Controller Exiting\n");
  rt_heap_delete(&rtHeap);
  free(rtSendMessage.data);
  free(rtReceiveMessage.data);
  exit(1);
}

int main(int argc, char *argv[])
{
  // signal handler for ctrl + c
  struct sigaction signalHandler;
  signalHandler.sa_handler = TerminationHandler;
  sigemptyset(&signalHandler.sa_mask);
  signalHandler.sa_flags = 0;
  sigaction(SIGINT, &signalHandler, NULL);

  printf("Connecting to motor\n");

  mlockall(MCL_CURRENT | MCL_FUTURE);

  cpu_set_t cpuSet;

  // TODO receive motor output
  CPU_ZERO(&cpuSet);
  CPU_SET(7, &cpuSet);
  rt_heap_create(&rtHeap, "rtControllerHeap", RtQueue::kMessageSize, H_SINGLE);

  rt_task_create(&rtReceiveMotorOutputTask, "rtControlReceiveMotorOutputTask",
    RtTask::kStackSize, RtTask::kHighPriority, RtTask::kMode);
  rt_task_set_affinity(&rtReceiveMotorOutputTask, &cpuSet);
  rt_task_start(&rtReceiveMotorOutputTask, ReceiveMotorOutputRoutine, NULL);

  // send motor input task
  rt_queue_create(&rtMotorInputQueue, "rtMotorInputQueue",
    RtQueue::kMessageSize * RtQueue::kQueueLimit, RtQueue::kQueueLimit, Q_FIFO);

  CPU_ZERO(&cpuSet);
  CPU_SET(6, &cpuSet);
  rt_task_create(&rtSendMotorInputTask, "rtSendMotorInputTask", RtTask::kStackSize,
    RtTask::kHighPriority, RtTask::kMode);
  rt_task_set_affinity(&rtSendMotorInputTask, &cpuSet);
  rt_task_start(&rtSendMotorInputTask, SendMotorInputRoutine, NULL);

  // receive motor input through message pipe
  CPU_ZERO(&cpuSet);
  CPU_SET(7, &cpuSet);
  rt_pipe_create(&rtPipe, "rtPipeRtp1", 1, RtMessage::kMessageSize);
  rt_task_create(&rtReceiveMotorInputFromPipeTask, "rtReceiveMotorInputFromPipeTask",
    RtTask::kStackSize, RtTask::kMediumPriority, RtTask::kMode);
  rt_task_set_affinity(&rtReceiveMotorInputFromPipeTask, &cpuSet);
  rt_task_start(&rtReceiveMotorInputFromPipeTask, ReceiveMotorInputFromPipeRoutine, NULL);

  for (;;)
  {}

  return 0;
}
