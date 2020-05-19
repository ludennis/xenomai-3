#ifndef _RTRESISTANCETASK_H_
#define _RTRESISTANCETASK_H_

#include <memory>

#include <RtMacro.h>
#include <RtPeriodicTask.h>
#include <RtSharedArray.h>

#include <PxiCardTask.h>

class RtResistanceTask: public RtPeriodicTask, public PxiCardTask
{
public:
  static std::shared_ptr<RtSharedArray> mRtSharedArray;

public:
  RtResistanceTask() = delete;
  RtResistanceTask(
    const char* name, const int stackSize, const int priority, const int mode, const int period);
  int StartRoutine();
  static void Routine(void*);
  ~RtResistanceTask();
};

#endif // _RTRESISTANCETASK_H_
