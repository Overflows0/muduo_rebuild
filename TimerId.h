#pragma once
#include "stdint.h"

class Timer;

/**
 * 单独用TimerId封装Timer的目的是为了将对Timer的操作和Timer数据分离，
 * 有的时候用户仅仅需要传递数据而不进行任何操作
 */
class TimerId
{
public:
    TimerId(Timer *timer = nullptr, int64_t seq = 0)
        : timer_(timer),
          sequence_(seq) 
    {
    }

    friend class TimerQueue;

private:
    Timer *timer_; //Timer指针可以被不同对象所共享持有
    int64_t sequence_;  //单个指针不足以区分定时器（可以被共享），序列号区分不同的持有者下的定时器编号，该变量具有原子性
};