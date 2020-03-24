#ifndef _INPUT_INTERFACE_H_
#define _INPUT_INTERFACE_H_

#include <chrono>

#include "SharedMsg.h"
#include "generated_model.h"
#include "generated_model_private.h"

namespace input_interface
{

MsgDynoCmd GetMsgDynoCmd();

MsgDynoSensing GetMsgDynoSensing();

MsgMcuOutput GetMsgMcuOutput();

MsgMotorOutput GetMsgMotorOutput();

void OutputMsgMotorOutput(const MsgMotorOutput& output);

} // namespace input_interface

#endif // _INPUT_INTERFACE_H_
