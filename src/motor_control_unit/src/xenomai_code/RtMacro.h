#ifndef _RTMACRO_H_
#define _RTMACRO_H_

namespace RtTask
{
constexpr auto kStackSize = 0;
constexpr auto kHighPriority = 80;
constexpr auto kMediumPriority = 50;
constexpr auto kMode = 0;
constexpr auto kMessageSize = 256u;
}

namespace RtTime
{
constexpr auto kTwoHundredMicroseconds = 200000;
constexpr auto kOneMillisecond = 1000000;
constexpr auto kTenMilliseconds = 10000000;
constexpr auto kHundredMilliseconds = 100000000;
constexpr auto kOneSecond = 1000000000;
constexpr auto kNanosecondsToMicroseconds = 1000;
constexpr auto kNanosecondsToMilliseconds = 1000000;
constexpr auto kNanosecondsToSeconds = 1000000000;
constexpr auto kMutexAcquireTimeout = 1000000; // 1 ms
}

namespace RtCpu
{
constexpr auto kCore4 = 4;
constexpr auto kCore5 = 5;
constexpr auto kCore6 = 6;
constexpr auto kCore7 = 7;
}

#endif // _RTMACRO_H_
