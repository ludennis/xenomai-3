#include <RtSharedArray.h>

RtSharedArray::RtSharedArray(const char* name, const RTIME &timeout)
  : mName(name)
  , mTimeout(timeout)
{
  rt_mutex_create(&mMutex, mName);
}

void RtSharedArray::Set(unsigned int index, DWORD element)
{
  rt_mutex_acquire_until(&mMutex, mTimeout);
  mArray[index] = element;
  rt_mutex_release(&mMutex);
}

void RtSharedArray::SetArray(const std::vector<DWORD> &elements)
{
  rt_mutex_acquire_until(&mMutex, mTimeout);
  for(auto i{0u}; i < elements.size(); ++i)
  {
    mArray[i] = elements[i];
  }
  rt_mutex_release(&mMutex);
}

DWORD RtSharedArray::Get(unsigned int index)
{
  rt_mutex_acquire_until(&mMutex, mTimeout);
  auto element = mArray[index];
  rt_mutex_release(&mMutex);
  return element;
}
