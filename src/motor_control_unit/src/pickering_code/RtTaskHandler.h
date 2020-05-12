#ifndef _RTTASKHANDLER_H_
#define _RTTASKHANDLER_H_

#include <stdio.h>

#include <alchemy/task.h>

#include <Pilpxi.h>

constexpr auto kTaskStackSize = 0;
constexpr auto kMediumTaskPriority = 50;
constexpr auto kTaskMode = 0;
constexpr auto kTaskPeriod = 20000; // 20 us
constexpr auto kNanosecondsToMicroseconds = 1000;
constexpr auto kNanosecondsToMilliseconds = 1000000;
constexpr auto kNanosecondsToSeconds = 1000000000;

class RtTaskHandler
{
public:
  static RT_TASK mRtTask;
  static RTIME mNow;
  static RTIME mPrevious;
  static RTIME mOneSecondTimer;

  static DWORD mBus;
  static DWORD mBuses[100];
  static DWORD mDevices[100];
  static DWORD mDevice;
  static DWORD mCardNum;
  static DWORD mData[100];
  static DWORD mSubunit;
  static DWORD mResistance;
  static DWORD mNumInputSubunits;
  static DWORD mNumOutputSubunits;
  static DWORD mNumOfFreeCards;

public:
  RtTaskHandler();
  int SetSubunitResistance();
  static void SetSubunitResistanceRoutine(void*);
};

#endif // _RTTASKHANDLER_H_
