#include <assert.h>

#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop)
    : baseLoop_(baseLoop),
      started_(false),
      numThreads_(0),
      next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::start()
{
    assert(!started_);
    baseLoop_->assertInLoopThread();

    started_ = true;
    for (int i = 0; i < numThreads_; i++)
    {
        EventLoopThread *thread = new EventLoopThread();
        loops_.push_back(thread->startLoop());
        /* 由线程池独占管理线程对象的生命期 */
        threads_.push_back(
            std::move(std::unique_ptr<EventLoopThread>(thread)));
    }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    EventLoop *loop = baseLoop_;

    /* Robin循环 - 每个loop的优先级都是平等的 */
    if (!loops_.empty())
    {
        loop = loops_[next_];
        next_++;
        if (static_cast<size_t>(next_) >= loops_.size())
            next_ = 0;
    }
    return loop;
}