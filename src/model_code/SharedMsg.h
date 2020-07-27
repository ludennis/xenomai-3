#ifndef _INPUT_SHAREDMSG_H_
#define _INPUT_SHAREDMSG_H_

#ifdef MATLAB_MEX_FILE
#include "tmwtypes.h"
#else
#include "rtwtypes.h"
#endif

/* TYPE DEFINE */
typedef struct {
    real32_T ft_DynoRPM;
} MsgDynoCmd;

typedef struct {
    real32_T ft_OutputTorqueS;
    real32_T ft_VoltageQ;
    real32_T ft_VoltageD;
    real32_T ft_CurrentUS;
    real32_T ft_CurrentVS;
    real32_T ft_CurrentWS;
} MsgDynoSensing;

typedef struct {
    real32_T ft_DutyUPhase;
    real32_T ft_DutyVPhase;
    real32_T ft_DutyWPhase;
} MsgMcuOutput;

typedef struct {
    real32_T ft_CurrentU;
    real32_T ft_CurrentV;
    real32_T ft_CurrentW;
    real32_T ft_RotorRPM;
    real32_T ft_RotorDegreeRad;
    real32_T ft_OutputTorque;
} MsgMotorOutput;


#endif /* _INPUT_SHAREDMSG_H_ */
