#include <RtSharedState.h>

RtSharedState::RtSharedState()
{
  // TODO: make name a memeber variable
  rt_mutex_create(&mMutex, "RtSharedStateMutex");
}

void RtSharedState::Set(bool b)
{
  // TODO: make time a member variable
  rt_mutex_acquire_until(&mMutex, TM_INFINITE);
  mState = b;
  rt_mutex_release(&mMutex);
}

BOOL RtSharedState::Get()
{
  // TODO: make time a member variable
  rt_mutex_acquire_until(&mMutex, TM_INFINITE);
  auto currentState = mState;
  rt_mutex_release(&mMutex);
  return currentState;
}
