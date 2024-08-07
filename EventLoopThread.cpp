#include <stdio.h>
#include <assert.h>

#include "EventLoopThread.h"

EventLoopThread::EventLoopThread()
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this)),
      mutex_(),
      cond_()
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != NULL)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    assert(!thread_.started);
    thread_.start(); // 从这里创建新线程

    if (loop_ == nullptr)
    {
        std::unique_lock<std::mutex> lk(mutex_);
        cond_.wait(lk, [this]
                   { return loop_ != nullptr; });
    }

    return loop_;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;

    {
        std::lock_guard<std::mutex> lk(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();
}