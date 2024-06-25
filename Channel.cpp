#include "poll.h"
#include "assert.h"

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
    assert(!eventHandling_);
}

void Channel::remove()
{
    loop_->removeChannel(this);
}

void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    eventHandling_ = true;
    if (revents_ & POLLNVAL)
    {
        LOG_ERROR("Channel::handelEvent() POLLNAVAL");
    }
    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_)
            errorCallback_();
    }
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if (closeCallback_)
            closeCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLHUP))
    {
        if (readCallback_)
            readCallback_(receiveTime);
    }
    if (revents_ & POLLOUT)
    {
        if (writeCallback_)
            writeCallback_();
    }
    eventHandling_ = false;
}