#include <RtSharedResistanceArray.h>

RtSharedResistanceArray::RtSharedResistanceArray()
{
  // TODO: change the naming, so no same name would appear
  rt_mutex_create(&mMutex, "RtSharedResistanceArrayMutex");
}

void RtSharedResistanceArray::Set(unsigned int i, DWORD resistance)
{
  rt_mutex_acquire_until(&mMutex, TM_INFINITE);
  mResistances[i] = resistance;
  rt_mutex_release(&mMutex);
}

void RtSharedResistanceArray::SetArray(const std::vector<DWORD> &resistances)
{
  rt_mutex_acquire_until(&mMutex, TM_INFINITE);
  for(auto i{0u}; i < resistances.size(); ++i)
  {
    mResistances[i] = resistances[i];
  }
  rt_mutex_release(&mMutex);
}

DWORD RtSharedResistanceArray::Get(unsigned int i)
{
  rt_mutex_acquire_until(&mMutex, TM_INFINITE);
  auto returnResistance = mResistances[i];
  rt_mutex_release(&mMutex);
  return returnResistance;
}
