#ifndef _RTSHAREDSTATE_H_
#define _RTSHAREDSTATE_H_

#include <alchemy/mutex.h>

#include <Pilpxi.h>

class RtSharedState
{
private:
  BOOL mState;
  RT_MUTEX mMutex;
  RTIME mTimeout;
  const char* mName;

public:
  RtSharedState() = delete;
  RtSharedState(const char* name, const RTIME &timeout=TM_INFINITE);
  void Set(bool b);
  BOOL Get();
};

#endif // _RTSHAREDSTATE_H_
