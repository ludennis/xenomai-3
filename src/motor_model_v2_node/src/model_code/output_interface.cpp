/* INCLUDE */
#include <memory>
#include <iostream>

#include "output_interface.h"
#include "SharedMsg.h"

/* VARIABLE */
std::shared_ptr<MsgMotorOutput> MotorOutputPtr;
std::shared_ptr<MsgDynoSensing> DynoSensingPtr;

/* FUNCTION */
void setMsgDynoCmd(MsgDynoCmd data)
{
    /* MsgDynoCmd */
}

void setMsgDynoSensing(MsgDynoSensing data)
{
    std::cout << "output_interface line 19" << std::endl;
    /* MsgDynoSensing */
    DynoSensingPtr->ft_OutputTorqueS = data.ft_OutputTorqueS;
    std::cout << "output_interface line 23" << std::endl;
    DynoSensingPtr->ft_VoltageQ = data.ft_VoltageQ;
    std::cout << "output_interface line 25" << std::endl;
    DynoSensingPtr->ft_VoltageD = data.ft_VoltageD;
    std::cout << "output_interface line 27" << std::endl;
    DynoSensingPtr->ft_CurrentUS = data.ft_CurrentUS;
    std::cout << "output_interface line 29" << std::endl;
    DynoSensingPtr->ft_CurrentVS = data.ft_CurrentVS;
    std::cout << "output_interface line 31" << std::endl;
    DynoSensingPtr->ft_CurrentWS = data.ft_CurrentWS;
}

void setMsgMcuOutput(MsgMcuOutput data)
{
    /* MsgMcuOutput */
}

void setMsgMotorOutput(MsgMotorOutput data)
{
    /* MsgMotorOutput */
MotorOutputPtr->ft_CurrentU = data.ft_CurrentU;
MotorOutputPtr->ft_CurrentV = data.ft_CurrentV;
MotorOutputPtr->ft_CurrentW = data.ft_CurrentW;
MotorOutputPtr->ft_RotorRPM = data.ft_RotorRPM;
MotorOutputPtr->ft_RotorDegreeRad = data.ft_RotorDegreeRad;
MotorOutputPtr->ft_OutputTorque = data.ft_OutputTorque;
}

void outputInterfaceInit(void)
{
    /* INIT */
    MotorOutputPtr = std::make_shared<MsgMotorOutput>();
    DynoSensingPtr = std::make_shared<MsgDynoSensing>();
}
