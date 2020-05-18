#ifndef _RTSWITCHTASK_H_
#define _RTSWITCHTASK_H_

#include <memory>

#include <RtSharedState.h>
#include <RtPeriodicTask.h>
#include <PxiCardTask.h>

constexpr auto kTaskStackSize = 0;
constexpr auto kHighTaskPriority = 80;
constexpr auto kMediumTaskPriority = 50;
constexpr auto kTaskMode = 0;
constexpr auto kTaskPeriod = 10000000; // 10 ms
constexpr auto kNanosecondsToMicroseconds = 1000;
constexpr auto kNanosecondsToMilliseconds = 1000000;
constexpr auto kNanosecondsToSeconds = 1000000000;
constexpr auto kMutexAcquireTimeout = 1000000; // 1 ms

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
