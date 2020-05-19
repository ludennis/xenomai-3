#ifndef _RTGENERATESTATETASK_H_
#define _RTGENERATESTATETASK_H_

#include <memory>

#include <testing.h>

#include <RtSharedState.h>
#include <RtPeriodicTask.h>
#include <PxiCardTask.h>

class RtGenerateStateTask: public RtPeriodicTask
{
public:
  static std::shared_ptr<RtSharedState> mRtSharedState;
  static testingModelClass mModel;

public:
  RtGenerateStateTask();
  RtGenerateStateTask(
    const char* name, const int stackSize, const int priority, const int mode, const int period);
  int StartRoutine();
  static void Routine(void*);
  ~RtGenerateStateTask();
};

#endif // _RTGENERATESTATETASK_H_
