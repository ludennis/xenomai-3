#include <sys/mman.h>

#include <limits>
#include <signal.h>
#include <iostream>

#include <alchemy/task.h>

#include <RtMacro.h>

RT_TASK rtMotorReceiveStepTask;
RT_TASK rtSendMotorStepTask;

RT_TASK_MCB rtSendMessage;
RT_TASK_MCB rtReceiveMessage;

RTIME rtTimerBegin;
RTIME rtTimerEnd;
RTIME rtTimerOneSecond;
RTIME rtTimerFiveSeconds;

unsigned int numberOfMessages{0u};
unsigned int numberOfMessagesRealTime{0u};
double totalRoundTripTime;
RTIME minRoundTripTime = std::numeric_limits<RTIME>::max();
RTIME maxRoundTripTime = std::numeric_limits<RTIME>::min();

void TerminationHandler(int s)
{
  printf("Controller Exiting\n");

  free(rtSendMessage.data);
  free(rtReceiveMessage.data);
  exit(1);
}

void SendMotorStepRoutine(void*)
{
  // find motor task
  rt_task_bind(&rtMotorReceiveStepTask, "rtMotorReceiveStepTask", TM_INFINITE);
  rt_printf("Found motor\n");

  rtTimerOneSecond = rt_timer_read();
  rtTimerFiveSeconds = rt_timer_read();

  for (;;)
  {
    // send message
    rtSendMessage.data = (char*) malloc(RtTask::kMessageSize);
    rtSendMessage.size = RtTask::kMessageSize;
    const char sendData[] = "step";
    memcpy(rtSendMessage.data, sendData, RtTask::kMessageSize);

    rtReceiveMessage.data = (char*) malloc(RtTask::kMessageSize);
    rtReceiveMessage.size = RtTask::kMessageSize;

    rtTimerBegin = rt_timer_read();
    auto retval = rt_task_send(&rtMotorReceiveStepTask, &rtSendMessage, &rtReceiveMessage,
      TM_INFINITE);
    rtTimerEnd = rt_timer_read();
    if (retval < 0)
    {
      rt_printf("rt_task_send error: %s\n", strerror(-retval));
    }
    else
    {
      if (rt_timer_read() - rtTimerFiveSeconds >= RtTime::kFiveSeconds)
      {
        auto timeElapsed = rtTimerEnd - rtTimerBegin;
        ++numberOfMessages;
        totalRoundTripTime += timeElapsed;

        if (timeElapsed < minRoundTripTime)
          minRoundTripTime = timeElapsed;
        else if (timeElapsed > maxRoundTripTime)
          maxRoundTripTime = timeElapsed;
        if (timeElapsed <= RtTime::kTenMicroseconds)
          ++numberOfMessagesRealTime;
      }
    }

    free(rtSendMessage.data);
    free(rtReceiveMessage.data);

    if (rt_timer_read() - rtTimerOneSecond >= RtTime::kOneSecond)
    {
      rt_printf("num msgs: %d, min: %d ns, avg: %.1f ns, max: %d ns, hrt: %.5f%\n",
        numberOfMessages, minRoundTripTime,
        totalRoundTripTime/numberOfMessages, maxRoundTripTime,
        (numberOfMessagesRealTime * 100.0f) / numberOfMessages);
      rtTimerOneSecond = rt_timer_read();
    }

    rt_task_wait_period(NULL);
  }
}

int main(int argc, char *argv[])
{
  // signal handler for ctrl + c
  struct sigaction signalHandler;
  signalHandler.sa_handler = TerminationHandler;
  sigemptyset(&signalHandler.sa_mask);
  signalHandler.sa_flags = 0;
  sigaction(SIGINT, &signalHandler, NULL);

  std::cout << "Connecting to motor ...";

  mlockall(MCL_CURRENT | MCL_FUTURE);

  cpu_set_t cpuSet;
  CPU_ZERO(&cpuSet);
  CPU_SET(5, &cpuSet);
  CPU_SET(6, &cpuSet);
  CPU_SET(7, &cpuSet);
  CPU_SET(8, &cpuSet);

  rt_task_create(&rtSendMotorStepTask, "rtSendMotorStepTask",
    RtTask::kStackSize, RtTask::kHighPriority, RtTask::kMode);
  rt_task_set_periodic(&rtSendMotorStepTask, TM_NOW,
    rt_timer_ns2ticks(RtTime::kTwentyMicroseconds));
  rt_task_set_affinity(&rtSendMotorStepTask, &cpuSet);
  rt_task_start(&rtSendMotorStepTask, SendMotorStepRoutine, NULL);

  for (;;)
  {}

  return 0;
}
