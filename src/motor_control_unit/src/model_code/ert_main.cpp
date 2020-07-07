#include <sys/mman.h>
#include <signal.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
#include <sstream>

#include <alchemy/task.h>
#include <alchemy/queue.h>

#include <RtMacro.h>
#include <MessageTypes.h>

#include "generated_model.h"
#include "input_interface.h"

RT_TASK rtMotorReceiveStepTask;
RT_TASK rtMotorBroadcastOutputTask;

RT_TASK_MCB rtReceiveMessage;
RT_TASK_MCB rtSendMessage;

RTIME rtTimerBegin;
RTIME rtTimerEnd;
RTIME rtTimerOneSecond;

RT_QUEUE rtQueue;
RT_QUEUE_INFO rtQueueInfo;

unsigned int numberOfMessages{0u};
double totalStepTime{0.0};

void MotorReceiveStepRoutine(void*)
{
  rt_printf("Motor Ready to Receive\n");

  rtTimerOneSecond = rt_timer_read();

  for (;;)
  {
    // wait to receive step
    rtReceiveMessage.data = (char*) malloc(RtMessage::kMessageSize);
    rtReceiveMessage.size = RtMessage::kMessageSize;
    auto retval = rt_task_receive(&rtReceiveMessage, TM_INFINITE);
    if (retval < 0)
    {
      rt_printf("rt_task_receive error: %s\n", strerror(-retval));
    }
    auto flowid = retval;

    // send ack message
    rtSendMessage.data = (char*) malloc(RtMessage::kMessageSize);
    rtSendMessage.size = RtMessage::kMessageSize;
    const char buffer[] = "ack";
    memcpy(rtSendMessage.data, buffer, RtMessage::kMessageSize);
    rt_task_reply(flowid, &rtSendMessage);

    // run step
    if (strcmp((char*)rtReceiveMessage.data, "motor_step") == 0)
    {
      rtTimerBegin = rt_timer_read();
      generated_model_step();
      rtTimerEnd = rt_timer_read();
      ++numberOfMessages;
      totalStepTime += (rtTimerEnd - rtTimerBegin);
    }

    if (rt_timer_read() - rtTimerOneSecond > RtTime::kOneSecond)
    {
      rt_printf("Motor Stepped %d times. avg step time: %.2f nanoseconds\n",
        numberOfMessages, totalStepTime / numberOfMessages);
      rtTimerOneSecond = rt_timer_read();
    }

    free(rtReceiveMessage.data);
    free(rtSendMessage.data);
  }
}

void MotorBroadcastOutputRoutine(void*)
{
  // find queue to send to

  for (;;)
  {
    auto queueBound = rt_queue_bind(&rtQueue, "rtQueue", TM_INFINITE);
    if (queueBound == 0)
      rt_printf("Queue Bound\n");

    rt_printf("Sending/broadcasting\n");
    // send/boradcast
    void *message = rt_queue_alloc(&rtQueue, sizeof(RtMessage::kMessageSize));
    if (message == NULL)
      rt_printf("rt_queue_alloc error\n");
    MotorMessage *motorMessageData =
      new MotorMessage{0, 3.0, 4.0, 5.0, 6000.0, 35.0, 400.0};
    memcpy(message, motorMessageData, sizeof(MotorMessage));

    // send message
    auto retval = rt_queue_send(&rtQueue, message, sizeof(MotorMessage), Q_NORMAL);
    if (retval < -1)
    {
      rt_printf("rt_queue_send error: %s\n", strerror(-retval));
    }
    else
      rt_printf("motor message sent\n");

    rt_task_wait_period(NULL);
  }
}

void terminationHandler(int signal)
{
  std::cout << "Motor Exiting ..." << std::endl;
  free(rtReceiveMessage.data);
  free(rtSendMessage.data);
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

  rt_task_create(&rtMotorReceiveStepTask, "rtMotorReceiveStepTask", RtTask::kStackSize,
    RtTask::kHighPriority, RtTask::kMode);
  rt_task_start(&rtMotorReceiveStepTask, MotorReceiveStepRoutine, NULL);
  rt_printf("rtMotorReceiveStepTask started\n");


  // TODO: use a different CPU for this task
  rt_task_create(&rtMotorBroadcastOutputTask, "rtMotorBroadcastOutputTask",
    RtTask::kStackSize, RtTask::kMediumPriority, RtTask::kMode);
  rt_task_set_periodic(&rtMotorBroadcastOutputTask, TM_NOW,
    rt_timer_ns2ticks(RtTime::kOneSecond));
  rt_task_start(&rtMotorBroadcastOutputTask, MotorBroadcastOutputRoutine, NULL);
  rt_printf("rtMotorBroadcastOutputTask started\n");

  for (;;)
  {}

  return 0;
}
