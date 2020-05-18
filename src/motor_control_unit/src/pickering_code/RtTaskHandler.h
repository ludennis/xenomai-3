#ifndef _RTTASKHANDLER_H_
#define _RTTASKHANDLER_H_

#include <memory>
#include <stdio.h>
#include <vector>

#include <alchemy/mutex.h>
#include <alchemy/task.h>

#include <Pilpxi.h>
#include <RtSharedResistanceArray.h>

constexpr auto kTaskStackSize = 0;
constexpr auto kHighTaskPriority = 80;
constexpr auto kMediumTaskPriority = 50;
constexpr auto kTaskMode = 0;
constexpr auto kTaskPeriod = 10000000; // 10 ms
constexpr auto kNanosecondsToMicroseconds = 1000;
constexpr auto kNanosecondsToMilliseconds = 1000000;
constexpr auto kNanosecondsToSeconds = 1000000000;
constexpr auto kMutexAcquireTimeout = 1000000; // 1 ms

class RtTaskHandler
{
public:
  static RT_TASK mRtTask;
  static RTIME mNow;
  static RTIME mPrevious;
  static RTIME mOneSecondTimer;

  static DWORD mBus;
  static DWORD mBuses[100];
  static DWORD mCardNum;
  static DWORD mData[100];
  static DWORD mDevices[100];
  static DWORD mDevice;
  static DWORD mNumInputSubunits;
  static DWORD mNumOfFreeCards;
  static DWORD mNumOutputSubunits;
  static DWORD mResistance;
  static DWORD mResistances[100];
  static DWORD mSubunit;

  static CHAR mCardId[100];

  static std::shared_ptr<RtSharedResistanceArray> mRtSharedResistanceArray;

public:
  RtTaskHandler();
  void OpenCard(DWORD cardNum);
  void ViewAllSubunits(DWORD cardNum);
  int StartSetSubunitResistanceRoutine();
  static void SetSubunitResistanceRoutine(void*);
};

#endif // _RTTASKHANDLER_H_
