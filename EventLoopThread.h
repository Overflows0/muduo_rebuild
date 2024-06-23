#pragma once

#include <condition_variable>
#include <mutex>

#include "Thread.h"
#include "EventLoop.h"
#include "noncopyable.h"

/**
 * one loop per thread, 事件循环需要在单个线程上进行，
 * 同时用户又需要得到对象地址，
 * 因此一个函数(threadFunc)来循环，一个函数(startLoop)返回对象地址
 */
class EventLoopThread : noncopyable
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop *startLoop(); // 启动线程并用条件锁等待对象构造，最后返回对象地址

private:
    void threadFunc(); // 线程主函数，构造EventLoop对象后通知并开始循环

    bool exiting_;
    EventLoop *loop_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
};