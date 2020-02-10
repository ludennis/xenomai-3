#ifndef _INPUT_INTERFACE_H_
#define _INPUT_INTERFACE_H_

#include "SharedMsg.h"

/* FUNCTION PROTOTYPE */
MsgDynoCmd getMsgDynoCmd();

MsgDynoSensing getMsgDynoSensing();

MsgMcuOutput getMsgMcuOutput();

MsgMotorOutput getMsgMotorOutput();


#endif