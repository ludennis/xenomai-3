#ifndef _RTSHAREDARRAY_H_
#define _RTSHAREDARRAY_H_

#include <vector>

#include <alchemy/mutex.h>

#include <Pilpxi.h>

class RtSharedArray
{
public:
  DWORD mArray[100];
  RT_MUTEX mMutex;

public:
  RtSharedArray();
  void Set(unsigned int index, DWORD element);
  void SetArray(const std::vector<DWORD> &elements);
  DWORD Get(unsigned int index);
};

#endif // _RTSHAREDARRAY_H_
