#pragma once

#include "stdint.h"
#include <string>

/**
 * -提供时间戳加法、比较的功能；
 * -提供合法性检查及字符串转化的功能；
 * -提供当前时间的功能；
 */

// 对Unix时间戳进行封装
class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpochArg);
    ~Timestamp();

    std::string toString() const;
    static Timestamp now();
    static Timestamp invalid() { return Timestamp(); }
    bool isValid() { return microSecondsSinceEpoch_ > 0; }

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    int64_t microSecondsSinceEpoch_;
};

// 提供时间戳比较的功能（目的是为了set<Timestamp,Timer*>的排序）
inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

// 提供时间戳加法的功能；
inline Timestamp addTime(Timestamp time, double seconds)
{
    int64_t delta = seconds * (Timestamp::kMicroSecondsPerSecond);
    return Timestamp(time.microSecondsSinceEpoch() + delta);
}