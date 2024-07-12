#include <sys/epoll.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <assert.h>

#include "EpollPoller.h"
#include "Channel.h"
#include "Logger.h"

const int kNew = -1;    //表明该Channel没有加入监听列表
const int kAdded = 1;   //表明该Channel在监听列表中，处于监听状态
const int kDeleted = 2; //表明该Channel在监听列表中，处于无监听状态

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_ERROR("EpollPoller::epoll_create1()");
    }
}

EpollPoller::~EpollPoller()
{
    ::close(epollfd_);  //析构时调用close()，关闭epollfd来析构epoll对象
}

Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    LOG_DEBUG("fd total count %u", channels_.count);
    int numEvents = ::epoll_wait(epollfd_,
                                 events_.data(),
                                 static_cast<int>(events_.size()),
                                 timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        fillActiveChannels(numEvents, activeChannels);
        /* 如果传入的监听列表空间刚满就扩充 */
        if (static_cast<int>(events_.size()) == numEvents)
        {
            events_.resize(2 * events_.size());
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("nothing happended");
    }
    else
    {
        if (savedErrno != EINTR)
            savedErrno = errno;
        LOG_DEBUG("EpollPoller::poll()");
    }
    return now;
}

void EpollPoller::updateChannel(Channel *channel)
{
    Poller::assertInLoopThread();
    LOG_DEBUG("fd = %d, index = %d, events = %d",
              channel->fd(),
              channel->index(),
              channel->events());
    const int index = channel->index();
    int fd = channel->fd();
    /* 对于没有监听的Channel */
    if (index == kNew || index == kDeleted)
    {
        /* 没有在监听列表中则加入到监听列表中 */
        if (index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }

        /* 在监听列表中但处于无监听状态 */
        if (index == kDeleted)
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }

        /* 标记为监听，将需要监听描述符加入epoll对象当中 */
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else    //对处于监听状态的描述符做处理
    {
        (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);

        /* 如果Channel设置为无监听状态，则移除epoll对象对该描述符的监听事件 */
        if (channel->isNoneEvents())
        {
            channel->set_index(kDeleted);
            update(EPOLL_CTL_DEL, channel);
        }
        /* 否则epoll对象就修改对该描述符的监听事件 */
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::removeChannel(Channel *channel)
{
    Poller::assertInLoopThread();
    int fd = channel->fd();
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvents());
    const int index = channel->index();
    assert(index == kAdded || index == kDeleted);

    /* 如果在监听列表当中，先移除对Channel的监听 */
    if (channel->index() == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }

    size_t n = channels_.erase(fd);
    assert(n == 1);
    (void)n;
    channel->set_index(kNew);
}

void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; i++)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
#ifndef NDEBUG //非发布版本执行以下断言
        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
#endif
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EpollPoller::update(int operation, Channel *channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.ptr = channel;
    event.events = channel->events();
    if (::epoll_ctl(epollfd_, operation, channel->fd(), &event) < 0)
    {
        LOG_ERROR("epoll_ctl operation = %s fd = %d",
                  opeationToString(operation),
                  channel->fd());
    }
}

const char *EpollPoller::opeationToString(int op)
{
    switch (op)
    {
    case EPOLL_CTL_ADD:
        return "ADD";
    case EPOLL_CTL_DEL:
        return "DEL";
    case EPOLL_CTL_MOD:
        return "MOD";
    default:
        assert(false && "ERROR op");
        return "Unknown Operation";
    }
}