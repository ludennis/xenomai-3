#include <RtSharedArray.h>

RtSharedArray::RtSharedArray()
{
  // TODO: change the naming, so no same name would appear
  rt_mutex_create(&mMutex, "RtSharedArrayMutex");
}

void RtSharedArray::Set(unsigned int index, DWORD element)
{
  rt_mutex_acquire_until(&mMutex, TM_INFINITE);
  mArray[index] = element;
  rt_mutex_release(&mMutex);
}

void RtSharedArray::SetArray(const std::vector<DWORD> &elements)
{
  rt_mutex_acquire_until(&mMutex, TM_INFINITE);
  for(auto i{0u}; i < elements.size(); ++i)
  {
    mArray[i] = elements[i];
  }
  rt_mutex_release(&mMutex);
}

DWORD RtSharedArray::Get(unsigned int index)
{
  rt_mutex_acquire_until(&mMutex, TM_INFINITE);
  auto element = mArray[index];
  rt_mutex_release(&mMutex);
  return element;
}
