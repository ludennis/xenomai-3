//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// File: generated_model.h
//
// Code generated for Simulink model 'generated_model'.
//
// Model version                  : 1.121
// Simulink Coder version         : 9.1 (R2019a) 23-Nov-2018
// C/C++ source code generated on : Fri Aug 23 14:16:54 2019
//
// Target selection: ert.tlc
// Embedded hardware selection: Intel->x86-64 (Linux 64)
// Code generation objective: Execution efficiency
// Validation result: Not run
//
#ifndef RTW_HEADER_generated_model_h_
#define RTW_HEADER_generated_model_h_
#include <cmath>
#include <float.h>
#include <string.h>
#ifndef generated_model_COMMON_INCLUDES_
# define generated_model_COMMON_INCLUDES_
#include "rtwtypes.h"
#include "rtw_continuous.h"
#include "rtw_solver.h"
#include "output_interface.h"
#include "SharedMsg.h"
#include "input_interface.h"
#endif                                 // generated_model_COMMON_INCLUDES_

#include "generated_model_types.h"
#include "rt_nonfinite.h"
#include "rtGetInf.h"

// Macros for accessing real-time model data structure
#ifndef rtmGetErrorStatus
# define rtmGetErrorStatus(rtm)        ((rtm)->errorStatus)
#endif

#ifndef rtmSetErrorStatus
# define rtmSetErrorStatus(rtm, val)   ((rtm)->errorStatus = (val))
#endif

#ifndef rtmGetStopRequested
# define rtmGetStopRequested(rtm)      ((rtm)->Timing.stopRequestedFlag)
#endif

#ifndef rtmSetStopRequested
# define rtmSetStopRequested(rtm, val) ((rtm)->Timing.stopRequestedFlag = (val))
#endif

#ifndef rtmGetStopRequestedPtr
# define rtmGetStopRequestedPtr(rtm)   (&((rtm)->Timing.stopRequestedFlag))
#endif

#ifndef rtmGetT
# define rtmGetT(rtm)                  (rtmGetTPtr((rtm))[0])
#endif

#ifndef rtmGetTPtr
# define rtmGetTPtr(rtm)               ((rtm)->Timing.t)
#endif

// Block signals and states (default storage) for system '<Root>'
typedef struct {
  MsgDynoSensing
    BusConversion_InsertedFor_MsgDynoSensing_at_inport_0_BusCreator1;
  MsgMotorOutput
    BusConversion_InsertedFor_MsgMotorOutput_at_inport_0_BusCreator1;
  MsgMcuOutput MsgMcuOutput_m;         // '<Root>/MsgMcuOutput'
  real_T SwitchBound;                  // '<S14>/Switch Bound'
  real_T SwitchBound_m;                // '<S11>/Switch Bound'
  real_T Sum1;                         // '<S6>/Sum1'
  real_T Gain3;                        // '<S7>/Gain3'
  real_T Switch;                       // '<S7>/Switch'
  real_T Ualfa;                        // '<S22>/U_alpha'
  real_T Ubeta;                        // '<S22>/U_beta'
  real_T daxis;                        // '<S22>/d-axis'
  real_T Product;                      // '<S7>/Product'
  real_T Sum5;                         // '<S9>/Sum5'
  real_T qaxis;                        // '<S22>/q-axis'
  real_T Sum5_k;                       // '<S10>/Sum5'
  real_T SwitchBound_d;                // '<S17>/Switch Bound'
  real_T Product1;                     // '<S7>/Product1'
  real_T UnitDelay1_DSTATE;            // '<S4>/Unit Delay1'
  real_T UnitDelay_DSTATE;             // '<S4>/Unit Delay'
  real_T Product5;                     // '<S10>/Product5'
  real_T Product1_n;                   // '<S9>/Product1'
  real32_T DataTypeConversion6;        // '<S1>/Data Type Conversion6'
  real32_T DataTypeConversion7;        // '<S1>/Data Type Conversion7'
} DW_generated_model_T;

// Continuous states (default storage)
typedef struct {
  real_T Integrator_CSTATE;            // '<S10>/Integrator'
  real_T Integrator_CSTATE_n;          // '<S9>/Integrator'
  real_T Integrator1_CSTATE;           // '<S7>/Integrator1'
  real_T Integrator_CSTATE_j;          // '<S7>/Integrator'
  real_T Integrator2_CSTATE;           // '<S7>/Integrator2'
} X_generated_model_T;

// State derivatives (default storage)
typedef struct {
  real_T Integrator_CSTATE;            // '<S10>/Integrator'
  real_T Integrator_CSTATE_n;          // '<S9>/Integrator'
  real_T Integrator1_CSTATE;           // '<S7>/Integrator1'
  real_T Integrator_CSTATE_j;          // '<S7>/Integrator'
  real_T Integrator2_CSTATE;           // '<S7>/Integrator2'
} XDot_generated_model_T;

// State disabled
typedef struct {
  boolean_T Integrator_CSTATE;         // '<S10>/Integrator'
  boolean_T Integrator_CSTATE_n;       // '<S9>/Integrator'
  boolean_T Integrator1_CSTATE;        // '<S7>/Integrator1'
  boolean_T Integrator_CSTATE_j;       // '<S7>/Integrator'
  boolean_T Integrator2_CSTATE;        // '<S7>/Integrator2'
} XDis_generated_model_T;

#ifndef ODE3_INTG
#define ODE3_INTG

// ODE3 Integration Data
typedef struct {
  real_T *y;                           // output
  real_T *f[3];                        // derivatives
} ODE3_IntgData;

#endif

// Real-time Model Data Structure
struct tag_RTM_generated_model_T {
  const char_T *errorStatus;
  RTWSolverInfo solverInfo;
  X_generated_model_T *contStates;
  int_T *periodicContStateIndices;
  real_T *periodicContStateRanges;
  real_T *derivs;
  boolean_T *contStateDisabled;
  boolean_T zCCacheNeedsReset;
  boolean_T derivCacheNeedsReset;
  boolean_T CTOutputIncnstWithState;
  real_T odeY[5];
  real_T odeF[3][5];
  ODE3_IntgData intgData;

  //
  //  Sizes:
  //  The following substructure contains sizes information
  //  for many of the model attributes such as inputs, outputs,
  //  dwork, sample times, etc.

  struct {
    int_T numContStates;
    int_T numPeriodicContStates;
    int_T numSampTimes;
  } Sizes;

  //
  //  Timing:
  //  The following substructure contains information regarding
  //  the timing information for the model.

  struct {
    uint32_T clockTick0;
    uint32_T clockTickH0;
    time_T stepSize0;
    uint32_T clockTick1;
    uint32_T clockTickH1;
    SimTimeStep simTimeStep;
    boolean_T stopRequestedFlag;
    time_T *t;
    time_T tArray[2];
  } Timing;
};

// Continuous states (default storage)
extern X_generated_model_T generated_model_X;

// Block signals and states (default storage)
extern DW_generated_model_T generated_model_DW;

#ifdef __cplusplus

extern "C" {

#endif

  // Model entry point functions
  extern void generated_model_initialize(void);
  extern void generated_model_step(void);
  extern void generated_model_terminate(void);

#ifdef __cplusplus

}
#endif

// Real-time Model object
#ifdef __cplusplus

extern "C" {

#endif

  extern RT_MODEL_generated_model_T *const generated_model_M;

#ifdef __cplusplus

}
#endif

//-
//  These blocks were eliminated from the model due to optimizations:
//
//  Block '<S4>/Gain2' : Unused code path elimination
//  Block '<S1>/Data Type Conversion' : Eliminate redundant data type conversion
//  Block '<S1>/Data Type Conversion1' : Eliminate redundant data type conversion
//  Block '<S1>/Data Type Conversion2' : Eliminate redundant data type conversion
//  Block '<S1>/Data Type Conversion4' : Eliminate redundant data type conversion
//  Block '<S1>/Signal Conversion' : Eliminate redundant signal conversion block
//  Block '<S1>/Signal Conversion1' : Eliminate redundant signal conversion block
//  Block '<S1>/Signal Conversion2' : Eliminate redundant signal conversion block
//  Block '<S1>/Signal Conversion3' : Eliminate redundant signal conversion block
//  Block '<S1>/Signal Conversion4' : Eliminate redundant signal conversion block
//  Block '<S1>/Signal Conversion5' : Eliminate redundant signal conversion block
//  Block '<S1>/Signal Conversion6' : Eliminate redundant signal conversion block
//  Block '<S1>/Signal Conversion7' : Eliminate redundant signal conversion block
//  Block '<S10>/Data Type Conversion' : Eliminate redundant data type conversion
//  Block '<S10>/Data Type Conversion1' : Eliminate redundant data type conversion
//  Block '<S10>/Data Type Conversion2' : Eliminate redundant data type conversion
//  Block '<S7>/Data Type Conversion' : Eliminate redundant data type conversion
//  Block '<S5>/Data Type Conversion' : Eliminate redundant data type conversion
//  Block '<S5>/Data Type Conversion1' : Eliminate redundant data type conversion
//  Block '<S2>/Constant2' : Unused code path elimination


//-
//  The generated code includes comments that allow you to trace directly
//  back to the appropriate location in the model.  The basic format
//  is <system>/block_name, where system is the system number (uniquely
//  assigned by Simulink) and block_name is the name of the block.
//
//  Use the MATLAB hilite_system command to trace the generated code back
//  to the model.  For example,
//
//  hilite_system('<S3>')    - opens system 3
//  hilite_system('<S3>/Kp') - opens and selects block Kp which resides in S3
//
//  Here is the system hierarchy for this model
//
//  '<Root>' : 'generated_model'
//  '<S1>'   : 'generated_model/MotorModel'
//  '<S2>'   : 'generated_model/MotorModel/Subsystem1'
//  '<S3>'   : 'generated_model/MotorModel/Subsystem1/PMSMModel'
//  '<S4>'   : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel'
//  '<S5>'   : 'generated_model/MotorModel/Subsystem1/PMSMModel/Scaling & Transformation'
//  '<S6>'   : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/Electrical  Equations'
//  '<S7>'   : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/Mechanical Equation'
//  '<S8>'   : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/dq2ABC'
//  '<S9>'   : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/Electrical  Equations/ Stator voltage equation for d-Axis'
//  '<S10>'  : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/Electrical  Equations/Stator voltage equation for q-Axis'
//  '<S11>'  : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/Electrical  Equations/ Stator voltage equation for d-Axis/Debar Zero Zone'
//  '<S12>'  : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/Electrical  Equations/ Stator voltage equation for d-Axis/Debar Zero Zone/Relation Sign'
//  '<S13>'  : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/Electrical  Equations/ Stator voltage equation for d-Axis/Debar Zero Zone/Unary Minus'
//  '<S14>'  : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/Electrical  Equations/Stator voltage equation for q-Axis/Debar Zero Zone'
//  '<S15>'  : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/Electrical  Equations/Stator voltage equation for q-Axis/Debar Zero Zone/Relation Sign'
//  '<S16>'  : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/Electrical  Equations/Stator voltage equation for q-Axis/Debar Zero Zone/Unary Minus'
//  '<S17>'  : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/Mechanical Equation/Debar Zero Zone'
//  '<S18>'  : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/Mechanical Equation/Debar Zero Zone/Relation Sign'
//  '<S19>'  : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/Mechanical Equation/Debar Zero Zone/Unary Minus'
//  '<S20>'  : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/dq2ABC/Inverse Clarke  Transformation'
//  '<S21>'  : 'generated_model/MotorModel/Subsystem1/PMSMModel/PMSMModel/dq2ABC/Inverse Park  Transformation'
//  '<S22>'  : 'generated_model/MotorModel/Subsystem1/PMSMModel/Scaling & Transformation/Clarke and Park'

#endif                                 // RTW_HEADER_generated_model_h_

//
// File trailer for generated code.
//
// [EOF]
//
