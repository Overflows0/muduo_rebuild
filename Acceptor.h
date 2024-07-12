#pragma once
#include <functional>

#include "EventLoop.h"
#include "noncopyable.h"
#include "Socket.h"

class InetAddress;
class Channel;

/* 等待并处理到来的连接请求，然后创建连接 */
class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr);
    ~Acceptor();

    void listen();
    bool listenning() { return listenning_; }
    void setNewConnectionCallback(const NewConnectionCallback &cb) { cb_ = std::move(cb); } // 设置连接回调

private:
    void handleRead(); // 一旦有连接到达，就调用accept()建立连接，并调用连接回调

    bool listenning_;
    Socket acceptSocket_; // 接收连接请求的socket
    EventLoop *loop_;
    Channel acceptChaneel_;
    NewConnectionCallback cb_;
    int idleFd_;
};