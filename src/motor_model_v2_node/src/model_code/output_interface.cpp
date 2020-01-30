/* INCLUDE */
#include "rclcpp/rclcpp.hpp"
#include "output_interface.h"

/* USING */
using stMsgMotorOutput = data_interfaces::msg::MsgMotorOutput;
using stMsgDynoSensing = data_interfaces::msg::MsgDynoSensing;

/* VARIABLE */
stMsgMotorOutput::SharedPtr outputData1;
stMsgDynoSensing::SharedPtr outputData2;

/* FUNCTION */
void setMsgDynoCmd(MsgDynoCmd data)
{
    /* MsgDynoCmd */
}

void setMsgDynoSensing(MsgDynoSensing data)
{
    /* MsgDynoSensing */
outputData2->ft_outputtorques = data.ft_OutputTorqueS;
outputData2->ft_voltageq = data.ft_VoltageQ;
outputData2->ft_voltaged = data.ft_VoltageD;
outputData2->ft_currentus = data.ft_CurrentUS;
outputData2->ft_currentvs = data.ft_CurrentVS;
outputData2->ft_currentws = data.ft_CurrentWS;
}

void setMsgMcuOutput(MsgMcuOutput data)
{
    /* MsgMcuOutput */
}

void setMsgMotorOutput(MsgMotorOutput data)
{
    /* MsgMotorOutput */
outputData1->ft_currentu = data.ft_CurrentU;
outputData1->ft_currentv = data.ft_CurrentV;
outputData1->ft_currentw = data.ft_CurrentW;
outputData1->ft_rotorrpm = data.ft_RotorRPM;
outputData1->ft_rotordegreerad = data.ft_RotorDegreeRad;
outputData1->ft_outputtorque = data.ft_OutputTorque;
}

void outputInterfaceInit(void)
{
    /* INIT */
    outputData1 = std::make_shared<stMsgMotorOutput>();
    outputData2 = std::make_shared<stMsgDynoSensing>();
}
