#ifndef _RTSHAREDSTATE_H_
#define _RTSHAREDSTATE_H_

#include <alchemy/mutex.h>

#include <Pilpxi.h>

class RtSharedState
{
private:
  BOOL mState;
  RT_MUTEX mMutex;

public:
  RtSharedState();
  void Set(bool b);
  BOOL Get();
};

#endif // _RTSHAREDSTATE_H_
