#pragma once

#include "assert.h"

#include "Poller.h"

class Channel;
class Timestamp;

class PollPoller : public Poller
{
public:
    PollPoller(EventLoop *loop);
    ~PollPoller();

    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;

    void updateChannel(Channel *channel) override;

private:
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
};