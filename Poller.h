#pragma once

#include <vector>
#include <unordered_map>
#include "poll.h"

#include "EventLoop.h"
#include "noncopyable.h"

class Channel;
class Timestamp;

// 该类是事件分发器的基类，为I/O多路复用核心模块保留统一的接口
// 由PollPoller和EpollPoller继承
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel *>;
    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;

    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;
    // virtual bool hasChannel() const;

    static Poller *newDefaultPoller(EventLoop *loop);
    void assertInLoopThread() { ownerLoop_->assertInLoopThread(); }

protected:
    using ChannelMap = std::unordered_map<int, Channel *>;
    using PollfdList = std::vector<pollfd>;
    ChannelMap channels_;
    PollfdList pollfds_;

private:
    EventLoop *ownerLoop_;
};