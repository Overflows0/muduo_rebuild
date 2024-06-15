#pragma once

#include <set>
#include <vector>

#include "Timestamp.h"
#include "Timer.h"
#include "Channel.h"

class EventLoop;
class Timer;
class TimerId;

class TimerQueue
{
public:
    TimerQueue(EventLoop *loop);

    ~TimerQueue();

    /* 任何线程都可以调用addTimer，但真正把Timer加入队列的只有原IO线程 */
    TimerId addTimer(Timestamp timestamp, const TimerCallback &cb, double interval);
    // void cancel(TimerId timerId);

private:
    using Entry = std::pair<Timestamp, Timer *>;
    using TimerList = std::set<Entry>;

    /* 加入的Timer如果到期时间比第一个早，就重新调整timerfd,优先调用该Timer */
    void addTimerInLoop(Timer *timer);

    /* timerfd在满足到期时间后就处理读事件 */
    void handleRead();

    std::vector<Entry> getExpired(Timestamp now);

    /*
     * 处理一批已执行的到期Timers，
     * 定期Timers重置到期时间并重新加入队列，
     * 一次性Timers则释放资源
     */
    void reset(std::vector<Entry> &&expired, Timestamp now);
    /* 插入timers_队列前检查到期时间的优先级并返回是否最先到期 */
    bool insert(Timer *timer);

    EventLoop *loop_;
    const int timerfd_;
    Channel timerChannel_;

    /* 用set来排序优先到期的定时器，
     * 通过时间戳获取Timer指针-> pair<Timestamp,Timer*>,
     * 用pair作为元素是因为Timerstamp有可能相同，而相同Timerstamp的pair的地址却不同
     */
    TimerList timers_;
};