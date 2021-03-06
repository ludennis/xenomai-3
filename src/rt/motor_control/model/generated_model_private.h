//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// File: generated_model_private.h
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
#ifndef RTW_HEADER_generated_model_private_h_
#define RTW_HEADER_generated_model_private_h_
#include "rtwtypes.h"

// Private macros used by the generated code to access rtModel
#ifndef rtmIsMajorTimeStep
# define rtmIsMajorTimeStep(rtm)       (((rtm)->Timing.simTimeStep) == MAJOR_TIME_STEP)
#endif

#ifndef rtmIsMinorTimeStep
# define rtmIsMinorTimeStep(rtm)       (((rtm)->Timing.simTimeStep) == MINOR_TIME_STEP)
#endif

#ifndef rtmSetTPtr
# define rtmSetTPtr(rtm, val)          ((rtm)->Timing.t = (val))
#endif

extern real_T rt_modd_snf(real_T u0, real_T u1);
extern real32_T rt_modf_snf(real32_T u0, real32_T u1);

// private model entry point functions
extern void generated_model_derivatives(void);

#endif                                 // RTW_HEADER_generated_model_private_h_

//
// File trailer for generated code.
//
// [EOF]
//
