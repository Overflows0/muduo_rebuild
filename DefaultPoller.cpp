
#include "Poller.h"
#include "PollPoller.h"

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    return new PollPoller(loop);
}