#include <vector>
#include <functional>
#include <mutex>
#include "assert.h"
#include "sys/eventfd.h"

#include "EventLoop.h"
#include "Poller.h"
#include "Logger.h"
#include "Channel.h"
#include "TimerId.h"
#include "Timer.h"

const int kPollTimeMs = 10000;
__thread EventLoop *t_loopInthisThread = 0;

static int createEventFd()
{
    int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd < 0)
    {
        LOG_ERROR("EventLoop::createEventFd() failed in ::eventfd()");
        abort();
    }
    return fd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFucntors_(false),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventFd()),
      wakeupChannel_(new Channel(this, wakeupFd_))
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
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    assert(!looping_);
    t_loopInthisThread = NULL;
    ::close(wakeupFd_);
}

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
        doPendingFunctors();
    }

    LOG_DEBUG("EventLoop %p stop looping.\n", this);
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(const Functor &cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor &cb)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    if (!isInLoopThread() || callingPendingFucntors_)
    {
        wakeup();
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    assertInLoopThread();
    assert(channel->ownerLoop() == this);
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    assertInLoopThread();
    assert(channel->ownerLoop() == this);
    poller_->removeChannel(channel);
}

TimerId EventLoop::runAt(const Timestamp &time, const TimerCallback &cb)
{
    assert(timerQueue_ != NULL);
    return timerQueue_->addTimer(time, cb, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback &cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback &cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(time, cb, interval);
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8.", n);
    }
}

EventLoop *EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInthisThread;
}

void EventLoop::abortNotInThread()
{
    LOG_FATAL("EventLoop::abortNotInThread - EventLoop%p was created in Thread = %d, current thread = %d\n", this, threadId_, CurrentThread::tid());
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFucntors_ = true;

    /**
     * 将全局的回调队列与自身堆栈的回调队列置换再执行，
     * 这样做能多出时间供其他线程加入回调，
     * 同时避免race condition
     */
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &func : functors)
    {
        func();
    }

    callingPendingFucntors_ = false;
}