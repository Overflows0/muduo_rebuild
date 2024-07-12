#pragma once

#include <functional>

#include "noncopyable.h"

class EventLoop;
class Timestamp;

/*
 * -用Channel对象封装文件描述符，一对一；
 * -持有者不局限于EventLoop对象；
 * -主要功能是注册事件，设置回调和处理事件
 */
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;
    Channel(EventLoop *loop, int fd);
    ~Channel();

    void tie(const std::shared_ptr<void> &obj);
    int fd() { return fd_; }
    int events() { return events_; }
    void set_revents(int rev) { revents_ = rev; }        // 供poller调用，设置实际事件
    bool isNoneEvents() { return events_ == kNonEvent; } // 告诉Poller忽略对该事件的监视

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
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    /* 标记自己在poller监听数组中的索引 */
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    EventLoop *ownerLoop() { return loop_; }

    void handleEvent(Timestamp receiveTime); // 调用handleEvent()对特定的活跃描述符检查每一个poll事件，然后执行回调

    void setReadCallback(const ReadEventCallback &cb)
    {
        readCallback_ = std::move(cb);
    }
    void setWriteCallback(const EventCallback &cb)
    {
        writeCallback_ = std::move(cb);
    }
    void setErrorCallback(const EventCallback &cb)
    {
        errorCallback_ = std::move(cb);
    }
    void setCloseCallback(const EventCallback &cb)
    {
        closeCallback_ = std::move(cb);
    }

    void remove();

private:
    void update(); // 通过EventLoop间接调用Poller来修改Channel
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNonEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    int fd_;
    int events_;
    int revents_;
    int index_;
    bool eventHandling_;
    bool tied_;
    std::weak_ptr<void> tie_;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;
};