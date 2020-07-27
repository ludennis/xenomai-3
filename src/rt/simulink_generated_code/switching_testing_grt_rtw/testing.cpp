/*
 * testing.cpp
 *
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 *
 * Code generation for model "testing".
 *
 * Model version              : 1.1
 * Simulink Coder version : 9.2 (R2019b) 18-Jul-2019
 * C++ source code generated on : Fri May  8 17:02:41 2020
 *
 * Target selection: grt.tlc
 * Note: GRT includes extra infrastructure and instrumentation for prototyping
 * Embedded hardware selection: Intel->x86-64 (Windows64)
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#include "testing.h"
#include "testing_private.h"

/* Model step function */
void testingModelClass::step()
{
  /* Outport: '<Root>/Out1' incorporates:
   *  Sin: '<Root>/Sine Wave'
   */
  testing_Y.Out1 = std::sin(testing_P.SineWave_Freq * (&testing_M)->Timing.t[0]
    + testing_P.SineWave_Phase) * testing_P.SineWave_Amp +
    testing_P.SineWave_Bias;

  /* Update absolute time for base rate */
  /* The "clockTick0" counts the number of times the code of this task has
   * been executed. The absolute time is the multiplication of "clockTick0"
   * and "Timing.stepSize0". Size of "clockTick0" ensures timer will not
   * overflow during the application lifespan selected.
   * Timer of this task consists of two 32 bit unsigned integers.
   * The two integers represent the low bits Timing.clockTick0 and the high bits
   * Timing.clockTickH0. When the low bit overflows to 0, the high bits increment.
   */
  if (!(++(&testing_M)->Timing.clockTick0)) {
    ++(&testing_M)->Timing.clockTickH0;
  }

  (&testing_M)->Timing.t[0] = (&testing_M)->Timing.clockTick0 * (&testing_M)
    ->Timing.stepSize0 + (&testing_M)->Timing.clockTickH0 * (&testing_M)
    ->Timing.stepSize0 * 4294967296.0;
}

/* Model initialize function */
void testingModelClass::initialize()
{
  /* Registration code */
  {
    /* Setup solver object */
    rtsiSetSimTimeStepPtr(&(&testing_M)->solverInfo, &(&testing_M)
                          ->Timing.simTimeStep);
    rtsiSetTPtr(&(&testing_M)->solverInfo, &rtmGetTPtr((&testing_M)));
    rtsiSetStepSizePtr(&(&testing_M)->solverInfo, &(&testing_M)
                       ->Timing.stepSize0);
    rtsiSetErrorStatusPtr(&(&testing_M)->solverInfo, (&rtmGetErrorStatus
      ((&testing_M))));
    rtsiSetRTModelPtr(&(&testing_M)->solverInfo, (&testing_M));
  }

  rtsiSetSimTimeStep(&(&testing_M)->solverInfo, MAJOR_TIME_STEP);
  rtsiSetSolverName(&(&testing_M)->solverInfo,"FixedStepDiscrete");
  rtmSetTPtr((&testing_M), &(&testing_M)->Timing.tArray[0]);
  (&testing_M)->Timing.stepSize0 = 0.2;

  /* external outputs */
  testing_Y.Out1 = 0.0;
}

/* Model terminate function */
void testingModelClass::terminate()
{
  /* (no terminate code required) */
}

/* Constructor */
testingModelClass::testingModelClass() : testing_M()
{
  /* Currently there is no constructor body generated.*/
}

/* Destructor */
testingModelClass::~testingModelClass()
{
  /* Currently there is no destructor body generated.*/
}

/* Real-Time Model get method */
RT_MODEL_testing_T * testingModelClass::getRTM()
{
  return (&testing_M);
}
