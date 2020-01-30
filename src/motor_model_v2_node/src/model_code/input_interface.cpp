/* INCLUDE */
#include <chrono>
#include "rclcpp/rclcpp.hpp"
#include "data_interfaces/msg/msg_mcu_output.hpp"
#include "data_interfaces/msg/msg_dyno_cmd.hpp"
#include "input_interface.h"

/* USING */
using stMsgMcuOutput = data_interfaces::msg::MsgMcuOutput;
using stMsgDynoCmd = data_interfaces::msg::MsgDynoCmd;

/* EXTERN */
extern stMsgMcuOutput::SharedPtr inputData1;
extern stMsgDynoCmd::SharedPtr inputData2;

/* FUNCTION */
MsgDynoCmd getMsgDynoCmd()
{
    MsgDynoCmd MsgDynoCmdRetData;
MsgDynoCmdRetData.ft_DynoRPM = inputData2->ft_dynorpm;
    return MsgDynoCmdRetData;
}

MsgDynoSensing getMsgDynoSensing()
{
    MsgDynoSensing MsgDynoSensingRetData;
    return MsgDynoSensingRetData;
}

MsgMcuOutput getMsgMcuOutput()
{
    MsgMcuOutput MsgMcuOutputRetData;
MsgMcuOutputRetData.ft_DutyUPhase = inputData1->ft_dutyuphase;
MsgMcuOutputRetData.ft_DutyVPhase = inputData1->ft_dutyvphase;
MsgMcuOutputRetData.ft_DutyWPhase = inputData1->ft_dutywphase;
    return MsgMcuOutputRetData;
}

MsgMotorOutput getMsgMotorOutput()
{
    MsgMotorOutput MsgMotorOutputRetData;
    return MsgMotorOutputRetData;
}


void getAAA(){}