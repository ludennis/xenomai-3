#ifndef _OUTPUT_INTERFACE_H_
#define _OUTPUT_INTERFACE_H_

/* INCLUDE */
//#include "data_interfaces/msg/msg_motor_output.hpp"
//#include "data_interfaces/msg/msg_dyno_sensing.hpp"
#include "SharedMsg.h"

/* USING */
//using stMsgMotorOutput = data_interfaces::msg::MsgMotorOutput;
//using stMsgDynoSensing = data_interfaces::msg::MsgDynoSensing;

/* EXTERN */
//extern stMsgMotorOutput::SharedPtr outputData1;
//extern stMsgDynoSensing::SharedPtr outputData2;

/* FUNCTION PROTOTYPE */
void setMsgDynoCmd(MsgDynoCmd data);

void setMsgDynoSensing(MsgDynoSensing data);

void setMsgMcuOutput(MsgMcuOutput data);

void setMsgMotorOutput(MsgMotorOutput data);

void outputInterfaceInit(void);

#endif