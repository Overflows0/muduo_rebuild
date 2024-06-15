#include "assert.h"
#include "poll.h"

#include "PollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include "Timestamp.h"

PollPoller::PollPoller(EventLoop *loop)
    : Poller(loop)
{
}

PollPoller::~PollPoller() = default;

// 调用系统调用poll()，得到活跃描述符集合pollfds_，然后填充进活跃channel集合中，并返回poll()时刻
// 复杂度为O(N)
Timestamp PollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    int numEvents = ::poll(pollfds_.data(), pollfds_.size(), timeoutMs);
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG_DEBUG("%d Events happended", numEvents);
        fillActiveChannels(numEvents, activeChannels);
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("Nothing happended");
    }
    else
    {
        LOG_ERROR("PollPoller::poll()");
    }
    return now;
}

/*
    只有在channel要注册事件的时候才添加pollfd，
    或更改事件时修改pollfd
    添加到channels_时复杂度为O(logN),修改时复杂度为O(1)
*/
void PollPoller::updateChannel(Channel *channel)
{
    assertInLoopThread();
    assert(channel != nullptr && "确认channel是否被构造");
    if (channel->index() < 0)
    {
        // channel注册事件监听——包括添加pollfd，添加<fd,channel>记录到channels_中
        assert(channels_.find(channel->fd()) == channels_.end() && "检查channel对应的pollfd不存在");
        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events()); // pollfd::events的数据类型是short
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size()) - 1;
        channel->set_index(idx);
        channels_[pfd.fd] = channel;
    }
    else
    {
        // channel修改事件监听
        assert(channels_.find(channel->fd()) != channels_.end() && "检查channel对应的pollfd存在");
        assert(channels_[channel->fd()] == channel && "检查fd和channel是否对应");
        int idx = channel->index();
        assert(0 <= idx && idx < static_cast<int>(pollfds_.size()) && "检查idx是否在pollfds范围内");
        struct pollfd &pfd = pollfds_[idx];
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if (channel->isNoneEvents())
        {
            // 忽略掉该描述符
            pfd.fd = -1;
            // pfd.fd= -channel->fd()-1;
        }
    }
}

// 遍历活跃描述符集合pollfds_，然后根据无序map<fd,channel*>channels_，设置revents值，将channel指针填充进activeChannels
// 本质上是将活跃描述符转化为活跃channel对象
// 复杂度为O(N)
void PollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (PollfdList::const_iterator pfd = pollfds_.begin(); pfd != pollfds_.end() && numEvents > 0; pfd++)
    {
        if (pfd->revents > 0)
        {
            numEvents--;
            ChannelMap::const_iterator ch = channels_.find(pfd->fd);
            assert(ch != channels_.end()); // 如果找不到，返回end迭代器
            Channel *channel = ch->second;
            assert(channel->fd() == pfd->fd); // 检查invariant
            channel->set_revents(pfd->revents);
            // 不必手动重置pfd->revents为0，因为每次调用poll()时都会重置pfd->revents
            activeChannels->push_back(channel);
        }
    }
}