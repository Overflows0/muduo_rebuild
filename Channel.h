#pragma once

#include "functional"

#include "noncopyable.h"

class EventLoop;

/*
 * -用Channel对象封装文件描述符，一对一；
 * -持有者不局限于EventLoop对象；
 * -主要功能是注册事件，设置回调和处理事件
 */
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    Channel(EventLoop *loop, int fd);
    ~Channel();

    int fd() { return fd_; }
    int events() { return events_; }
    void set_revents(int rev) { revents_ = rev; } // 供poller调用，设置实际事件
    bool isNoneEvents() { return revents_ == kNonEvent; }

    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
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

    /* 标记自己在poller监听数组中的索引 */
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    EventLoop *ownerLoop() { return loop_; }

    void handleEvent(); // 调用handleEvent()对特定的活跃描述符检查每一个poll事件，然后执行回调

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
    void update(); // 通过EventLoop间接调用Poller来修改Channel

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