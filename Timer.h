#pragma once

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
          repeat_(interval_ > 0.0)
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

private:
    TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
};