#include <RtSharedState.h>

RtSharedState::RtSharedState(const char* name, const RTIME &timeout)
  : mName(name)
  , mTimeout(timeout)
{
  rt_mutex_create(&mMutex, mName);
}

void RtSharedState::Set(bool b)
{
  rt_mutex_acquire_until(&mMutex, mTimeout);
  mState = b;
  rt_mutex_release(&mMutex);
}

BOOL RtSharedState::Get()
{
  rt_mutex_acquire_until(&mMutex, mTimeout);
  auto currentState = mState;
  rt_mutex_release(&mMutex);
  return currentState;
}
