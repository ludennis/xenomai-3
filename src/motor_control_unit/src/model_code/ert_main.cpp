#include <sys/mman.h>
#include <signal.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
#include <sstream>

#include <alchemy/task.h>
#include <alchemy/queue.h>
#include <alchemy/heap.h>

#include <RtMacro.h>
#include <MessageTypes.h>

#include "generated_model.h"
#include "input_interface.h"

RT_HEAP rtHeap;

RT_TASK rtMotorStepTask;
RT_TASK rtMotorReceiveTask;
RT_TASK rtMotorBroadcastOutputTask;
RT_TASK rtMotorReceiveInputTask;

RT_TASK_MCB rtReceiveMessage;
RT_TASK_MCB rtSendMessage;

RTIME rtTimerBegin;
RTIME rtTimerEnd;
RTIME rtTimerOneSecond;

RT_QUEUE rtMotorInputQueue;
RT_QUEUE rtMotorOutputQueue;
RT_QUEUE_INFO rtMotorOutputQueueInfo;

unsigned int numberOfMessages{0u};
double totalStepTime{0.0};

void MotorStepRoutine(void*)
{
  for (;;)
  {
    rtTimerBegin = rt_timer_read();
    generated_model_step();
    rtTimerEnd = rt_timer_read();
    ++numberOfMessages;
    totalStepTime += (rtTimerEnd - rtTimerBegin);

    if (rt_timer_read() - rtTimerOneSecond > RtTime::kOneSecond)
    {
      rt_printf("Motor Stepped %d times. avg step time: %.2f nanoseconds\n",
        numberOfMessages, totalStepTime / numberOfMessages);
      rtTimerOneSecond = rt_timer_read();
    }

    rt_task_wait_period(NULL);
  }
}

void MotorReceiveInputRoutine(void*)
{
  rt_printf("MotorReceiveInputRoutine started\n");

  void *blockPointer;
  rt_heap_alloc(&rtHeap, RtQueue::kMessageSize, TM_INFINITE, &blockPointer);
  for (;;)
  {
    auto sendingQueueBound = rt_queue_bind(
      &rtMotorInputQueue, "rtMotorInputQueue", TM_INFINITE);
    if (sendingQueueBound != 0)
    {
      rt_printf("Sending queue binding error\n");
      continue;
    }

    rt_printf("Reading queue\n");
    auto bytesRead =
      rt_queue_read(&rtMotorInputQueue, blockPointer, RtQueue::kMessageSize, TM_INFINITE);

    if (bytesRead > 0)
    {
      rt_printf("Received %d\n", bytesRead);
      int messageType;
      memcpy(&messageType, blockPointer, sizeof(int));

      if (messageType == tMotorInputMessage)
      {
        MotorInputMessage motorInputMessage;
        memcpy(&motorInputMessage, blockPointer, sizeof(MotorInputMessage));
        rt_printf("Motor Input Message received: ft_OutputTorqueS = %f, "
          "ft_VoltageQ = %f, ft_VoltageD = %f\n", motorInputMessage.ft_OutputTorqueS,
          motorInputMessage.ft_VoltageQ, motorInputMessage.ft_VoltageD);
      }
    }

    rt_heap_free(&rtHeap, blockPointer);
  }
}

void MotorBroadcastOutputRoutine(void*)
{
  // find queue to send to
  for (;;)
  {
    rt_queue_inquire(&rtMotorOutputQueue, &rtMotorOutputQueueInfo);
    if (rtMotorOutputQueueInfo.nwaiters > 0)
    {
      // send/boradcast
      void *message = rt_queue_alloc(&rtMotorOutputQueue, sizeof(RtQueue::kMessageSize));
      if (message == NULL)
        rt_printf("rt_queue_alloc error\n");

      // TODO prevent two tasks figting over model
      auto generatedModelMotorOutput = input_interface::GetMsgMotorOutput();
      MotorOutputMessage motorOutputMessageData = MotorOutputMessage{
        tMotorOutputMessage, generatedModelMotorOutput.ft_CurrentU,
        generatedModelMotorOutput.ft_CurrentV, generatedModelMotorOutput.ft_CurrentW,
        generatedModelMotorOutput.ft_RotorRPM, generatedModelMotorOutput.ft_RotorDegreeRad,
        generatedModelMotorOutput.ft_OutputTorque};
      memcpy(message, &motorOutputMessageData, sizeof(MotorOutputMessage));

      // send message
      auto retval = rt_queue_send(
        &rtMotorOutputQueue, message, sizeof(MotorOutputMessage), Q_BROADCAST);
      if (retval < -1)
      {
        rt_printf("rt_queue_send error: %s\n", strerror(-retval));
      }
    }

    rt_task_wait_period(NULL);
  }
}

void terminationHandler(int signal)
{
  std::cout << "Motor Exiting ..." << std::endl;
  free(rtReceiveMessage.data);
  free(rtSendMessage.data);
  rt_queue_delete(&rtMotorOutputQueue);
  generated_model_terminate();
  exit(1);
}

int main(int argc, char * argv[])
{
  struct sigaction action;
  action.sa_handler = terminationHandler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, NULL);

  mlockall(MCL_CURRENT|MCL_FUTURE);

  generated_model_initialize();

  rt_queue_create(&rtMotorOutputQueue, "rtMotorOutputQueue", RtQueue::kMessageSize,
    RtQueue::kQueueLimit, Q_FIFO);
  rt_queue_inquire(&rtMotorOutputQueue, &rtMotorOutputQueueInfo);
  rt_printf("Queue %s created\n", rtMotorOutputQueueInfo.name);

  cpu_set_t cpuSet;
  CPU_ZERO(&cpuSet);
  CPU_SET(5, &cpuSet);
  rt_task_create(&rtMotorStepTask, "rtMotorStepTask", RtTask::kStackSize,
    RtTask::kHighPriority, RtTask::kMode);
  rt_task_set_affinity(&rtMotorStepTask, &cpuSet);
  rt_task_set_periodic(&rtMotorStepTask, TM_NOW, rt_timer_ns2ticks(RtTime::kTenMicroseconds));
  rt_task_start(&rtMotorStepTask, MotorStepRoutine, NULL);

  rt_heap_create(&rtHeap, "rtHeapMotorInput", RtQueue::kMessageSize, H_SINGLE);
  CPU_ZERO(&cpuSet);
  CPU_SET(6, &cpuSet);
  rt_task_create(&rtMotorReceiveInputTask, "rtMotorReceiveInputTask", RtTask::kStackSize,
    RtTask::kHighPriority, RtTask::kMode);
  rt_task_set_affinity(&rtMotorReceiveInputTask, &cpuSet);
  rt_task_start(&rtMotorReceiveInputTask, MotorReceiveInputRoutine, NULL);

  // TODO: use a different CPU for this task
  CPU_ZERO(&cpuSet);
  CPU_SET(7, &cpuSet);

  rt_task_create(&rtMotorBroadcastOutputTask, "rtMotorBroadcastOutputTask",
    RtTask::kStackSize, RtTask::kMediumPriority, RtTask::kMode);
  rt_task_set_periodic(&rtMotorBroadcastOutputTask, TM_NOW,
    rt_timer_ns2ticks(RtTime::kTenMilliseconds));
  rt_task_start(&rtMotorBroadcastOutputTask, MotorBroadcastOutputRoutine, NULL);
  rt_printf("rtMotorBroadcastOutputTask started\n");

  for (;;)
  {}

  return 0;
}
