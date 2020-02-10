/* INCLUDE */
#include <chrono>
#include "input_interface.h"

auto constexpr kDummyDynoRPM = 5.5;
auto constexpr kDummyDutyUPhase = 1.5;
auto constexpr kDummyDutyVPhase = 0.3;
auto constexpr kDummyDutyWPhase = 1.1;

/* FUNCTION */
MsgDynoCmd getMsgDynoCmd()
{
    MsgDynoCmd MsgDynoCmdRetData;
    MsgDynoCmdRetData.ft_DynoRPM = kDummyDynoRPM;
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
    MsgMcuOutputRetData.ft_DutyUPhase = kDummyDutyUPhase;
    MsgMcuOutputRetData.ft_DutyVPhase = kDummyDutyVPhase;
    MsgMcuOutputRetData.ft_DutyWPhase = kDummyDutyWPhase;
    return MsgMcuOutputRetData;
}

MsgMotorOutput getMsgMotorOutput()
{
    MsgMotorOutput MsgMotorOutputRetData;
    return MsgMotorOutputRetData;
}


void getAAA(){}