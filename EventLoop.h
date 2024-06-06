#pragma once

#include <vector>
#include <functional>
#include <memory>

#include "CurrentThread.h"

class Channel;
class Poller;

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    EventLoop *getEventLoopOfCurrentThread();
    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInThread();
        }
    }
    bool isInLoopThread() const { return CurrentThread::tid() == threadId_; }

private:
    void abortNotInThread();

    using ChannelList = std::vector<Channel *>;

    ChannelList activeChannels_;
    std::unique_ptr<Poller> poller_;
    bool looping_;
    bool quit_;
    const pid_t threadId_;
};