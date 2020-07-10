#ifndef _MESSAGETYPES_H_
#define _MESSAGETYPES_H_

// TODO: ensure a way that all message type size is less than RtMessage::kMessageSize
auto constexpr tMotorOutputMessage = 0;
auto constexpr tMotorInputMessage = 1;

struct MotorOutputMessage
{
  int messageType;
  float ft_CurrentU;
  float ft_CurrentV;
  float ft_CurrentW;
  float ft_RotorRPM;
  float ft_RotorDegreeRad;
  float ft_OutputTorque;
};

struct MotorInputMessage
{
  int messageType;
  float ft_OutputTorqueS;
  float ft_VoltageQ;
  float ft_VoltageD;
  float ft_CurrentUS;
  float ft_CurrentVS;
  float ft_CurrentWs;
};

#endif // _MESSAGETYPES_H_
