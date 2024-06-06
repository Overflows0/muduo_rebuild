#pragma once

#include "functional"

#include "noncopyable.h"

class EventLoop;

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    Channel(EventLoop *loop, int fd);
    ~Channel();

    int fd() { return fd_; }
    int events() { return events_; }
    void set_revents(int rev) { revents_ = rev; }
    bool isNoneEvents() { return revents_ == kNonEvent; }

    void enableReading()
    {
        events_ = kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ = kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll()
    {
        events_ = kNonEvent;
        update();
    }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    EventLoop *ownerLoop() { return loop_; }

    void handleEvent();
    void setReadCallback(const EventCallback &cb)
    {
        readCallback_ = cb;
    }
    void setWriteCallback(const EventCallback &cb)
    {
        writeCallback_ = cb;
    }
    void setErrorCallback(const EventCallback &cb)
    {
        errorCallback_ = cb;
    }

private:
    void update();

    static const int kNonEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    int fd_;
    int events_;
    int revents_;
    int index_;

    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
};