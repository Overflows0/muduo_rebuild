#include <vector>
#include <functional>
#include "assert.h"

#include "EventLoop.h"
#include "Poller.h"
#include "Logger.h"
#include "Channel.h"
#include "Timestamp.h"

const int kPollTimeMs = 10000;
__thread EventLoop *t_loopInthisThread = 0;

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      activeChannels_(NULL)
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

// 简易loop，等待poll监听，通过poll填充活跃channel，然后处理事件
void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while (!quit_)
    {
        activeChannels_.clear();
        poller_->poll(kPollTimeMs, &activeChannels_);
        for (ChannelList::iterator it = activeChannels_.begin(); it != activeChannels_.end(); it++)
        {
            (*it)->handleEvent();
        }
    }
    LOG_DEBUG("EventLoop %p stop looping.\n", this);
    looping_ = false;
}

// 非IO线程也可以调用，quit_只是标志，下一次循环才会起效
void EventLoop::quit()
{
    quit_ = true;
}

// 调用poller_来管理channel，而不必再过多关注channel
void EventLoop::updateChannel(Channel *channel)
{
    assertInLoopThread();
    assert(channel->ownerLoop() == this);
    poller_->updateChannel(channel);
}

EventLoop *EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInthisThread;
}

void EventLoop::abortNotInThread()
{
    LOG_FATAL("EventLoop::abortNotInThread - EventLoop%p was created in Thread = %d, current thread = %d\n", this, threadId_, CurrentThread::tid());
}