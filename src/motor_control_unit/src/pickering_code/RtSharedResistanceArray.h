#ifndef _RTSHAREDRESISTANCEARRAY_H_
#define _RTSHAREDRESISTANCEARRAY_H_

#include <vector>

#include <alchemy/mutex.h>

#include <Pilpxi.h>

class RtSharedResistanceArray
{
public:
  DWORD mResistances[100];
  RT_MUTEX mMutex;

public:
  RtSharedResistanceArray();
  void Set(unsigned int i, DWORD resistance);
  void SetArray(const std::vector<DWORD> &resistance);
  DWORD Get(unsigned int i);
};

#endif // _RTSHAREDRESISTANCEARRAY_H_
