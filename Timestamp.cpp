#include <sys/time.h>
#include <inttypes.h>

#include "Timestamp.h"

//无参数构造函数构造无效定时器
Timestamp::Timestamp()
    : microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpochArg)
    : microSecondsSinceEpoch_(microSecondsSinceEpochArg) {}

Timestamp::~Timestamp() {}

Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Timestamp((seconds * kMicroSecondsPerSecond) + tv.tv_usec);
}

std::string Timestamp::toString() const
{
    char buf[32];
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microSeconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds, microSeconds);
    return buf;
}

std::string Timestamp::toFormattedString() const
{
    char buf[64];
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    int microSeconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);
    snprintf(buf, sizeof(buf), "%4d/%02d/%02d %02d:%02d:%02d.%06d",
             tm_time.tm_year + 1900,
             tm_time.tm_mon + 1,
             tm_time.tm_mday,
             tm_time.tm_hour,
             tm_time.tm_min,
             tm_time.tm_sec,
             microSeconds);
    return buf;
}