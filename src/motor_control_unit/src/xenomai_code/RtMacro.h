#ifndef _RTMACRO_H_
#define _RTMACRO_H_

namespace RtTask
{
constexpr auto kStackSize = 0;
constexpr auto kHighPriority = 80;
constexpr auto kMediumPriority = 50;
constexpr auto kMode = 0;
}

namespace RtTime
{
constexpr auto kTenMicroseconds = 10000;
constexpr auto kFifteenMicroseconds = 15000;
constexpr auto kTwentyMicroseconds = 20000;
constexpr auto kFiftyMicroseconds = 50000;
constexpr auto kOneHundredMicroseconds = 100000;
constexpr auto kTwoHundredMicroseconds = 200000;
constexpr auto kOneMillisecond = 1000000;
constexpr auto kTenMilliseconds = 10000000;
constexpr auto kHundredMilliseconds = 100000000;
constexpr auto kOneSecond = 1000000000;
constexpr auto kFiveSeconds = 5000000000;
constexpr auto kTenSeconds = 10000000000;
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
constexpr auto kMessageSize = 256u;
}

namespace RtQueue
{
constexpr auto kQueueLimit = 10;
}

#endif // _RTMACRO_H_
