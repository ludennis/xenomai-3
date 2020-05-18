#include <alchemy/task.h>

class RtPeriodicTask
{
public:
  static RT_TASK mRtTask;
  static RTIME mNow;
  static RTIME mPrevious;
  static RTIME mOneSecondTimer;

public:
  RtPeriodicTask();
  int StartRoutine();
  static void Routine(void*);
};
