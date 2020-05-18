#ifndef _RTSWITCHTASK_H_
#define _RTSWITCHTASK_H_

#include <memory>

#include <RtMacro.h>
#include <RtSharedState.h>
#include <RtPeriodicTask.h>
#include <PxiCardTask.h>

class RtSwitchTask: public RtPeriodicTask, public PxiCardTask
{
public:
  static std::shared_ptr<RtSharedState> mRtSharedState;

public:
  RtSwitchTask();
  int StartRoutine();
  static void Routine(void*);
};

#endif // _RTSWITCHTASK_H_
