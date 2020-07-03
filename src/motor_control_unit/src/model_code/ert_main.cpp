#include <sys/mman.h>
#include <signal.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
#include <sstream>

#include <alchemy/task.h>

#include <RtMacro.h>

#include "generated_model.h"
#include "input_interface.h"

RT_TASK rtMotorReceiveStepTask;

RT_TASK_MCB rtReceiveMessage;
RT_TASK_MCB rtSendMessage;

namespace {

//void motorStep()
//{
//  generated_model_step();
//}
//
//void OutputMsgMotorOutput(const MsgMotorOutput& output)
//{
//  std::cout << "MsgMotorOutput: ft_CurrentU = " << output.ft_CurrentU
//    << ", ft_CurrentV = " << output.ft_CurrentV << ", ft_CurrentW = " << output.ft_CurrentW
//    << ", ft_RotorRPM = " << output.ft_RotorRPM << ", ft_RotorDegreeRad = "
//    << output.ft_RotorDegreeRad << ", ft_OutputTorque = " << output.ft_OutputTorque
//    << std::endl;
//}

} // namespace

void MotorReceiveStepRoutine(void*)
{
  rt_printf("Motor Ready to Receive\n");

  for (;;)
  {
    // wait to receive step
    rtReceiveMessage.data = (char*) malloc(RtTask::kMessageSize);
    rtReceiveMessage.size = RtTask::kMessageSize;
    auto retval = rt_task_receive(&rtReceiveMessage, TM_INFINITE);
    if (retval < 0)
    {
      rt_printf("rt_task_receive error: %s\n", strerror(-retval));
    }
    auto flowid = retval;
    rt_printf("Received: %s\n", rtReceiveMessage.data);

    // send ack message
    rtSendMessage.data = (char*) malloc(RtTask::kMessageSize);
    rtSendMessage.size = RtTask::kMessageSize;
    const char buffer[] = "ack";
    memcpy(rtSendMessage.data, buffer, RtTask::kMessageSize);
    rt_task_reply(flowid, &rtSendMessage);
    rt_printf("Reply Sent: %s\n", rtSendMessage.data);

    // run step

    free(rtReceiveMessage.data);
    free(rtSendMessage.data);
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

  for (;;)
  {}

  return 0;
}
