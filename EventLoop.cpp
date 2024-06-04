#include "vector"
#include "functional"

#include "EventLoop.h"
#include "assert.h"
#include "Logger.h"
#include "poll.h"

__thread EventLoop *t_loopInthisThread = 0;

EventLoop::EventLoop()
    : looping_(false),
      threadId_(CurrentThread::tid())
{
    LOG_DEBUG("EventLoop created %p in this thread%d.\n", this, threadId_);
    if (t_loopInthisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread%d.\n", t_loopInthisThread, threadId_);
    }
    else
    {
        t_loopInthisThread = this;
    }
}

EventLoop::~EventLoop()
{
    assert(!looping_);
    t_loopInthisThread = NULL;
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;

    poll(NULL, 0, 5 * 1000);
    LOG_DEBUG("EventLoop %p stop looping.\n", this);
    looping_ = false;
}

EventLoop *EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInthisThread;
}

void EventLoop::abortNotInThread()
{
    LOG_FATAL("EventLoop::abortNotInThread - EventLoop%p was created in Thread = %d, current thread = %d\n", this, threadId_, CurrentThread::tid());
}