#pragma once

class Timer;

/**
 * 单独用TimerId封装Timer的目的是为了将对Timer的操作和Timer数据分离，
 * 有的时候用户仅仅需要传递数据而不进行任何操作
 */
class TimerId
{
public:
    explicit TimerId(Timer *timer)
        : value_(timer)
    {
    }

private:
    Timer *value_;
};