#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <mutex>

#include "CurrentThread.h"
#include "Callbacks.h"
#include "Timestamp.h"
#include "TimerQueue.h"

class Channel;
class Poller;
class TimerId;

class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();

    /**
     * 每次循环都重置channel，
     * 然后阻塞于poller监听描述符，
     * 如果有活跃描述符出现，就进行事务逻辑处理，
     * 最后再执行回调函数队列.
     */
    void loop();
    void quit(); // 其他线程也可以调用，修改标志延迟quit（目的是完成当前循环的所有任务）

    /**
     * -如果是本IO线程，就立即执行该回调;
     * -如果不是，则调用queueInLoop。
     */
    void runInLoop(const Functor &cb);
    /**
     * -将回调加入队列，让所绑定的线程执行
     * -如果是本线程调用该函数，且没有正在执行已有回调（即，正在执行IO），就加入队列延迟执行该回调
     * -如果是其他线程调用该函数，就加入队列并唤醒该线程执行回调
     */
    void queueInLoop(const Functor &cb);

    /**
     * 调用Poller更新维护或移除Channel，而不再过多关注
     */
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    /**
     * -可以通过调用定时器来执行回调函数；
     * -runAt()指定特定时间戳执行回调；
     * -runAfter()指定延时的时间段之后(从当前运行时计时)执行回调；
     * -runEvery()指定定期间隔循环执行回调。
     */
    TimerId runAt(const Timestamp &time, const TimerCallback &cb);
    TimerId runAfter(double delay, const TimerCallback &cb);
    TimerId runEvery(double interval, const TimerCallback &cb);
    void cancel(TimerId timerId);

    void wakeup(); // 任何其他线程都能唤醒该EventLoop

    EventLoop *getEventLoopOfCurrentThread(); // 返回当前执行线程原先绑定的EventLoop对象
    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInThread();
        }
    }
    bool isInLoopThread() const { return CurrentThread::tid() == threadId_; } // 确认当前执行线程是本线程

private:
    void abortNotInThread();
    void handleRead(); // 被唤醒时触发该读事件
    void doPendingFunctors();
    using ChannelList = std::vector<Channel *>;

    bool looping_;
    bool quit_;
    const pid_t threadId_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_; // 一个EventLoop只能持有一个wakeupChannel
    ChannelList activeChannels_;
    std::unique_ptr<Poller> poller_; // 一个EventLoop只能持有一个poller
    bool callingPendingFucntors_;
    std::unique_ptr<TimerQueue> timerQueue_; // 一个EventLoop只能持有一个timerQueue
    std::mutex mutex_;
    std::vector<Functor> pendingFunctors_; // pendingFunctors_是多生产者单消费者问题
};