#include <signal.h>
#include <sys/mman.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

#include <alchemy/heap.h>
#include <alchemy/queue.h>
#include <alchemy/task.h>

#include <MessageTypes.h>
#include <RtMacro.h>

#include "generated_model.h"
#include "input_interface.h"

RTIME rtTimerBegin;
RTIME rtTimerEnd;
RTIME rtTimerOneSecond;
RT_HEAP rtHeap;
RT_QUEUE rtMotorInputQueue;
RT_QUEUE rtMotorOutputQueue;
RT_QUEUE_INFO rtMotorOutputQueueInfo;
RT_TASK rtMotorBroadcastOutputTask;
RT_TASK rtMotorReceiveInputTask;
RT_TASK rtMotorStepTask;

auto numberOfMessages{0u};
double totalStepTime{0.0};

void terminationHandler(int signal)
{
  std::cout << "Motor Exiting ..." << std::endl;
  rt_queue_delete(&rtMotorOutputQueue);
  generated_model_terminate();
  exit(1);
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

      #ifdef MOTOR_CONTROL_DEBUG
      if (message == NULL)
        rt_printf("[motor|model] rt_queue_alloc error\n");
      #endif // MOTOR_CONTROL_DEBUG

      // TODO prevent two tasks figting over model
      auto generatedModelMotorOutput = input_interface::GetMsgMotorOutput();
      MotorOutputMessage motorOutputMessageData = MotorOutputMessage{
        tMotorOutputMessage, rt_timer_read(), generatedModelMotorOutput.ft_CurrentU,
        generatedModelMotorOutput.ft_CurrentV, generatedModelMotorOutput.ft_CurrentW,
        generatedModelMotorOutput.ft_RotorRPM, generatedModelMotorOutput.ft_RotorDegreeRad,
        generatedModelMotorOutput.ft_OutputTorque};
      memcpy(message, &motorOutputMessageData, sizeof(MotorOutputMessage));

      // send message
      auto retval = rt_queue_send(
        &rtMotorOutputQueue, message, sizeof(MotorOutputMessage), Q_BROADCAST);
      if (retval < -1)
      {
        rt_printf("[motor|model] rt_queue_send error: %s\n", strerror(-retval));
      }
    }

    rt_task_wait_period(NULL);
  }
}

void MotorReceiveInputRoutine(void*)
{
  rt_printf("[motor|model] MotorReceiveInputRoutine started\n");

  void *blockPointer;
  rt_heap_alloc(&rtHeap, RtQueue::kMessageSize, TM_INFINITE, &blockPointer);
  for (;;)
  {
    auto sendingQueueBound = rt_queue_bind(
      &rtMotorInputQueue, "rtMotorInputQueue", TM_INFINITE);
    if (sendingQueueBound != 0)
    {
      rt_printf("[motor|model] Sending queue binding error\n");
      continue;
    }

    #ifdef MOTOR_CONTROL_DEBUG
    rt_printf("[motor|model] Reading queue\n");
    #endif // MOTOR_CONTROL_DEBUG
    auto bytesRead =
      rt_queue_read(&rtMotorInputQueue, blockPointer, RtQueue::kMessageSize, TM_INFINITE);

    if (bytesRead > 0)
    {
      #ifdef MOTOR_CONTROL_DEBUG
      rt_printf("[motor|model] Received %d\n", bytesRead);
      #endif // MOTOR_CONTROL_DEBUG
      int messageType;
      memcpy(&messageType, blockPointer, sizeof(int));

      if (messageType == tMotorInputMessage)
      {
        MotorInputMessage motorInputMessage;
        memcpy(&motorInputMessage, blockPointer, sizeof(MotorInputMessage));
        #ifdef MOTOR_CONTROL_DEBUG
        rt_printf("[motor|model] Motor Input Message received: timestamp: %lld, ft_OutputTorqueS = %f, "
          "ft_VoltageQ = %f, ft_VoltageD = %f\n", motorInputMessage.timestamp,
          motorInputMessage.ft_OutputTorqueS, motorInputMessage.ft_VoltageQ,
          motorInputMessage.ft_VoltageD);

        rt_printf("[motor|model] Motor Input took %lld ns from rt pipe\n",
          rt_timer_read() - motorInputMessage.timestamp);
        #endif // MOTOR_CONTROL_DEBUG
      }
    }

    rt_heap_free(&rtHeap, blockPointer);
  }
}

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
      #ifdef MOTOR_CONTROL_DEBUG
      rt_printf("[motor|model] Motor Stepped %d times. avg step time: %.2f nanoseconds\n",
        numberOfMessages, totalStepTime / numberOfMessages);
      #endif // MOTOR_CONTROL_DEBUG
      rtTimerOneSecond = rt_timer_read();
    }

    rt_task_wait_period(NULL);
  }
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

  cpu_set_t cpuSet;

  // motor step task
  rt_task_create(&rtMotorStepTask, "rtMotorStepTask", RtTask::kStackSize,
    RtTask::kHighPriority, RtTask::kMode);

  CPU_ZERO(&cpuSet);
  CPU_SET(5, &cpuSet);
  rt_task_set_affinity(&rtMotorStepTask, &cpuSet);

  rt_task_set_periodic(&rtMotorStepTask, TM_NOW, rt_timer_ns2ticks(RtTime::kTenMicroseconds));
  rt_task_start(&rtMotorStepTask, MotorStepRoutine, NULL);

  // receive motor input task
  rt_heap_create(&rtHeap, "rtHeapMotorInput", RtQueue::kMessageSize, H_SINGLE);
  rt_task_create(&rtMotorReceiveInputTask, "rtMotorReceiveInputTask", RtTask::kStackSize,
    RtTask::kHighPriority, RtTask::kMode);

  CPU_ZERO(&cpuSet);
  CPU_SET(6, &cpuSet);
  rt_task_set_affinity(&rtMotorReceiveInputTask, &cpuSet);

  rt_task_start(&rtMotorReceiveInputTask, MotorReceiveInputRoutine, NULL);

  // broadcast motor output task
  rt_queue_create(&rtMotorOutputQueue, "rtMotorOutputQueue", RtQueue::kMessageSize,
    RtQueue::kQueueLimit, Q_FIFO);
  rt_queue_inquire(&rtMotorOutputQueue, &rtMotorOutputQueueInfo);
  rt_printf("[motor|model] Queue %s created\n", rtMotorOutputQueueInfo.name);

  rt_task_create(&rtMotorBroadcastOutputTask, "rtMotorBroadcastOutputTask",
    RtTask::kStackSize, RtTask::kMediumPriority, RtTask::kMode);

  CPU_ZERO(&cpuSet);
  CPU_SET(7, &cpuSet);
  rt_task_set_affinity(&rtMotorBroadcastOutputTask, &cpuSet);

  rt_task_set_periodic(&rtMotorBroadcastOutputTask, TM_NOW,
    rt_timer_ns2ticks(RtTime::kTenMilliseconds));
  rt_task_start(&rtMotorBroadcastOutputTask, MotorBroadcastOutputRoutine, NULL);
  rt_printf("[motor|model] rtMotorBroadcastOutputTask started\n");

  for (;;)
  {}

  return 0;
}
