#ifndef _MESSAGETYPES_H_
#define _MESSAGETYPES_H_

// TODO: ensure a way that all message type size is less than RtMessage::kMessageSize
auto constexpr tMotorMessage = 0;

struct MotorMessage
{
  int messageType;
  float ft_CurrentU;
  float ft_CurrentV;
  float ft_CurrentW;
  float ft_RotorRPM;
  float ft_RotorDegreeRad;
  float ft_OutputTorque;
};

#endif // _MESSAGETYPES_H_
