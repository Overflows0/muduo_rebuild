#pragma once

#include <assert.h>

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
    void removeChannel(Channel *channel) override;

private:
    using PollfdList = std::vector<pollfd>;
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    PollfdList pollfds_;
};