#ifndef _RTMACRO_H_
#define _RTMACRO_H_

namespace RtTask
{
constexpr auto kStackSize = 0;
constexpr auto kHighPriority = 80;
constexpr auto kMediumPriority = 50;
constexpr auto kLowPriority = 20;
constexpr auto kMode = 0;
constexpr auto kMessageSize = 8u;
}

namespace RtTime
{
constexpr auto kOneMicrosecond = 1000ull;
constexpr auto kTwoMicroseconds = 2000ull;
constexpr auto kFiveMicroseconds = 5000ull;
constexpr auto kTenMicroseconds = 10000ull;
constexpr auto kFifteenMicroseconds = 15000ull;
constexpr auto kTwentyMicroseconds = 20000ull;
constexpr auto kFiftyMicroseconds = 50000ull;
constexpr auto kOneHundredMicroseconds = 100000ull;
constexpr auto kTwoHundredMicroseconds = 200000ull;
constexpr auto kOneMillisecond = 1000000ull;
constexpr auto kTenMilliseconds = 10000000ull;
constexpr auto kHundredMilliseconds = 100000000ull;
constexpr auto kOneSecond = 1000000000ull;
constexpr auto kFiveSeconds = 5000000000ull;
constexpr auto kTenSeconds = 10000000000ull;
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

namespace RtMessage
{
constexpr auto kMessageSize = 40u;
}

namespace RtQueue
{
constexpr auto kQueueLimit = 10;
constexpr auto kMessageSize = 40u;
}

#endif // _RTMACRO_H_
