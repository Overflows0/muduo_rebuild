#pragma once

#include <vector>
#include <unordered_map>
#include <poll.h>

#include "EventLoop.h"
#include "noncopyable.h"

class Channel;
class Timestamp;

// 该类是事件分发器的基类，为I/O多路复用核心模块保留统一的接口
// 由PollPoller和EpollPoller继承
// 默认调用EpollPoller
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

    /**
     *  对于channels_,
     *  在EpollPoller仅仅用来断言确认Channel和fd的存在和对应关系
     *  在PollPoller中方便寻找活跃fd和Channel，因为Poll会返回所有描述符，需要自己查找
    */
    ChannelMap channels_; 


private:
    EventLoop *ownerLoop_;
};