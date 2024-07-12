#include <assert.h>
#include <sys/timerfd.h>
#include <cstring>
#include <algorithm>

#include "Timestamp.h"
#include "TimerQueue.h"
#include "TimerId.h"
#include "EventLoop.h"
#include "Logger.h"

/* 创建非阻塞的timerfd */
int createTimerfd()
{
    int timerFd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerFd < 0)
    {
        LOG_FATAL("Failed in timerfd_create");
    }
    return timerFd;
}

/* 如果Timerfd到期，就触发该读事件函数 */
void readTimerfd(int timerFd, Timestamp now)
{
    uint64_t howMany;
    ssize_t n = ::read(timerFd, &howMany, sizeof(howMany));
    LOG_DEBUG("TimerQueue::handleRead() %d at %s ", timerFd, now.toString().c_str());
    if (n != sizeof(howMany))
    {
        LOG_ERROR("TimerQueue::handleRead() read %lu bytes instead of 8", n);
    }
}

/* 将到期时间Timestamp 转化成 timespec */
struct timespec howMuchTimeFromNow(Timestamp when)
{
    /* 假设when没到期 */
    int64_t microSeconds = when.microSecondsSinceEpoch() -
                           Timestamp::now().microSecondsSinceEpoch();
    if (microSeconds < 100)
    {
        microSeconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(
        microSeconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(
        (microSeconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

/* 修改timerfd到期时间 */
void resetTimerfd(int timerFd, Timestamp when)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof(newValue));
    memset(&oldValue, 0, sizeof(oldValue));
    newValue.it_value = howMuchTimeFromNow(when);
    int ret = ::timerfd_settime(timerFd, 0, &newValue, &oldValue);
    if (ret)
    {
        LOG_INFO("timerfd_settime()");
    }
}

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerChannel_(loop, timerfd_),
      callingExpiredTimers_(false),
      timers_(),
      activeTimers_(),
      cancelingTimers_()
{
    timerChannel_.setReadCallback(
        std::bind(&TimerQueue::handleRead, this));
    timerChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    ::close(timerfd_);
    for (auto &it : timers_)
    {
        delete it.second; // 这里delete掉指针后不用置零，因为后续如果有有程序调用该指针时，还能清楚错误位置
    }
}

TimerId TimerQueue::addTimer(Timestamp when, const TimerCallback &cb, double interval)
{
    Timer *timer = new Timer(when, cb, interval);
    loop_->runInLoop(
        std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::addTimerInLoop(Timer *timer)
{
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);
    if (earliestChanged)
    {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(std::bind(
        &TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
    loop_->assertInLoopThread();
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if (it != activeTimers_.end())
    {
        Entry removing(it->first->expiration(), it->first);
        size_t n = timers_.erase(removing);

        assert(n == 1);
        (void)n;
        delete it->first;
        activeTimers_.erase(it);
    }
    else if (callingExpiredTimers_)
    {
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);

    /* timerfd一响应就从回调函数队列调取到期的回调函数 */
    std::vector<Entry> expired = getExpired(now);

    cancelingTimers_.clear();
    callingExpiredTimers_ = true;
    for (const Entry &it : expired)
    {
        it.second->run();
    }
    callingExpiredTimers_ = false;

    reset(std::move(expired), now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;

    /* 设定一个当前时间的哨兵值，得到第一个不早于它的Timer迭代器 */
    TimerQueue::Entry sentry = std::make_pair(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
    TimerList::iterator last = timers_.lower_bound(sentry);
    assert(last == timers_.end() || now < last->first);
    auto start = timers_.begin();

    /* 将到期的Timer转移到vector数组中并返回,用右值语义效率更好 */
    // std::copy(start, last, std::back_inserter(expired));
    std::move(std::make_move_iterator(start), std::make_move_iterator(last),
              std::back_inserter(expired));
    timers_.erase(start, last);

    std::for_each(expired.begin(), expired.end(), [&](Entry &it)
                  {
        ActiveTimer timer(it.second,it.second->sequence());
        size_t n=activeTimers_.erase(timer);
        assert(n==1);(void)n; });

    assert(timers_.size() == activeTimers_.size());
    return expired;
}

void TimerQueue::reset(std::vector<Entry> &&expired, Timestamp now)
{
    Timestamp nextExpired;
    for (Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        if (it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end())
        {
            it.second->restart(now);
            insert(it.second);
        }
        else
        {
            delete it.second;
        }
    }

    /* 比较最先到期的原定时器，如果新定时器先到期就重置timerfd_ */
    if (!timers_.empty())
    {
        nextExpired = timers_.begin()->second->expiration();
    }
    if (nextExpired.isValid())
    {
        resetTimerfd(timerfd_, nextExpired);
    }
}

bool TimerQueue::insert(Timer *timer)
{
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    if (timers_.begin() == timers_.end() || when < timers_.begin()->first)
        earliestChanged = true;
    {
        std::pair<TimerList::const_iterator, bool> result =
            timers_.insert(std::make_pair(when, timer));
        assert(result.second);
        (void)result;
    }
    {
        std::pair<ActiveTimerSet::const_iterator, bool> result =
            activeTimers_.insert(std::make_pair(timer, timer->sequence()));
        assert(result.second);
        (void)result;
    }
    return earliestChanged;
}
