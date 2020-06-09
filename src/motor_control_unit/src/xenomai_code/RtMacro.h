#ifndef _RTMACRO_H_
#define _RTMACRO_H_

namespace RtMacro
{

constexpr auto kTaskStackSize = 0;
constexpr auto kHighTaskPriority = 80;
constexpr auto kMediumTaskPriority = 50;
constexpr auto kTaskMode = 0;
constexpr auto kOneMsTaskPeriod = 1000000; // 1 ms
constexpr auto kTenMsTaskPeriod = 10000000; // 10 ms
constexpr auto kHundredMsTaskPeriod = 100000000; //100 ms
constexpr auto kOneSecondTaskPeriod = 1000000000; // 1000 ms
constexpr auto kNanosecondsToMicroseconds = 1000;
constexpr auto kNanosecondsToMilliseconds = 1000000;
constexpr auto kNanosecondsToSeconds = 1000000000;
constexpr auto kMutexAcquireTimeout = 1000000; // 1 ms

constexpr auto kCoreId4 = 4;
constexpr auto kCoreId5 = 5;
constexpr auto kCoreId6 = 6;
constexpr auto kCoreId7 = 7;

} // namespace RtMacro

#endif // _RTMACRO_H_
