#pragma once
#include <atomic>

#include "noncopyable.h"
#include "Timestamp.h"
#include "Callbacks.h"

/**
 * -Timer是对到期时间和回调函数的封装；
 * -提供定期执行的功能；
 */
class Timer : noncopyable
{
public:
    Timer(Timestamp when, const TimerCallback &cb, double interval)
        : callback_(std::move(cb)),
          expiration_(when),
          interval_(interval),
          repeat_(interval_ > 0.0),
          sequence_(Timer::seq_increAndGet())
    {
    }
    ~Timer() {}

    void run() const
    {
        callback_();
    }
    // 定期Timer可重置时间；
    void restart(Timestamp now);

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    // 线程安全，生成序列号
    static int64_t seq_increAndGet() { return seq_.fetch_add(1); }

private:
    TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    int64_t sequence_;
    static std::atomic<int64_t> seq_;
};