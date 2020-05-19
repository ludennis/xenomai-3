#ifndef _RTGENERATERESISTANCEARRAYTASK_H_
#define _RTGENERATERESISTANCEARRAYTASK_H_

#include <memory>
#include <vector>

#include <alchemy/task.h>

#include <RtPeriodicTask.h>
#include <RtSharedResistanceArray.h>

#include <testing.h>

class RtGenerateResistanceArrayTask : public RtPeriodicTask
{
public:
  static std::shared_ptr<RtSharedResistanceArray> mRtSharedResistanceArray;
  static testingModelClass mModel;
public:
  RtGenerateResistanceArrayTask() = delete;
  RtGenerateResistanceArrayTask(
    const char* name, const int stackSize, const int priority, const int mode, const int period);
  int StartRoutine();
  static void Routine(void*);
  ~RtGenerateResistanceArrayTask();
};

#endif // _RTGENERATERESISTANCEARRAYTASK_H_
