#include "poll.h"

#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"

const int Channel::kNonEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1)
{
}

Channel::~Channel()
{
}

// 通过EventLoop间接调用Poller来修改Channel
void Channel::update()
{
    loop_->updateChannel(this);
}

// 调用handleEvent()对特定的活跃描述符检查每一个poll事件，然后执行回调或报错
void Channel::handleEvent()
{
    if (revents_ & POLLNVAL)
    {
        LOG_ERROR("Channel::handelEvent() POLLNAVAL");
    }
    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_)
            errorCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLHUP))
    {
        if (readCallback_)
            readCallback_();
    }
    if (revents_ & POLLOUT)
    {
        if (writeCallback_)
            writeCallback_();
    }
}