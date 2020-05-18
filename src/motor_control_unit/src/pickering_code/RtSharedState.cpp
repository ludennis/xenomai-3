#include <RtSharedState.h>

RtSharedState::RtSharedState()
{
  rt_mutex_create(&mMutex, "RtSharedStateMutex");
}

void RtSharedState::Set(bool b)
{
  rt_mutex_acquire_until(&mMutex, TM_INFINITE);
  mState = b;
  rt_mutex_release(&mMutex);
}

BOOL RtSharedState::Get()
{
  rt_mutex_acquire_until(&mMutex, TM_INFINITE);
  auto currentState = mState;
  rt_mutex_release(&mMutex);
  return currentState;
}
