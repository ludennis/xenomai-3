#include "input_interface.h"

auto constexpr kDummyDynoRPM = 5.5;

namespace input_interface
{

MsgDynoCmd GetMsgDynoCmd()
{
  MsgDynoCmd MsgDynoCmdRetData;
  MsgDynoCmdRetData.ft_DynoRPM = kDummyDynoRPM;
  return MsgDynoCmdRetData;
}

MsgDynoSensing GetMsgDynoSensing()
{
  return generated_model_DW.BusConversion_InsertedFor_MsgDynoSensing_at_inport_0_BusCreator1;
}

MsgMcuOutput GetMsgMcuOutput()
{
  return generated_model_DW.MsgMcuOutput_m;
}

MsgMotorOutput GetMsgMotorOutput()
{
  return generated_model_DW.BusConversion_InsertedFor_MsgMotorOutput_at_inport_0_BusCreator1;
}

void getAAA(){}

} // namespace input_interface
