#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <string>

#include "TcpConnection.h"
#include "EventLoop.h"
#include "Callbacks.h"
#include "noncopyable.h"

class Acceptor;
class InetAddress;
class TcpConnection;

/* TcpServer负责处理连接请求，创建和关闭TcpConnection */
class TcpServer : noncopyable
{
public:
    TcpServer(EventLoop *loop, const InetAddress &listenAddr);
    ~TcpServer(); // 析构时移除剩下的TcpConnection记录

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connCb_ = cb;
    }
    void setMessageCallback(const MessageCallback &cb)
    {
        messaCb_ = cb;
    }

    void start(); // 服务器初始化连接监听连接请求的到来

private:
    /* 创建TcpConnection，设置回调，添加connection记录，开启对socket的监听*/
    void newConnection(int sockfd, const InetAddress &peerAddr);
    /* 移除对connection的记录，延后移除Channel(延后是因为有可能还有剩下的IO事务未处理) */
    void removeConnection(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_; // TcpServer持有loop_，但不决定loop的生死
    const std::string name_;
    bool started_;
    std::unique_ptr<Acceptor> acceptor_; // 只有TcpServer才能持有acceptor
    ConnectionCallback connCb_;
    MessageCallback messaCb_;
    int nextConnId_;
    ConnectionMap connections_; // 记录TcpConnection，以便检索来管理TcpConnection生命期
};