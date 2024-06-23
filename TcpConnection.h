#pragma once

#include <memory>
#include <functional>
#include <string>

#include "Callbacks.h"
#include "InetAddress.h"
#include "noncopyable.h"

class Channel;
class Socket;
class EventLoop;

class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                  std::string name,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connCb_ = cb;
    }
    void setMessageCallback(const MessageCallback &cb)
    {
        messaCb_ = cb;
    }
    void setCloseCallback(const CloseCallback &cb)
    {
        closeCb_ = cb;
    }

    bool connected() const { return state_ == kConnected; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() { return localAddr_; }
    const InetAddress &peerAddress() { return peerAddr_; }

    // 开启TcpConnection对socket的监听读事件，并执行用户级连接回调
    void connEstablished();
    // 通知用户关闭连接，关闭对socket的监听，并移除对应的channel
    void connDestroyed();

private:
    enum StateE // 进行特定操作时需要有特定连接状态的必要条件
    {
        kConnecting,   // 未连接，接下来准备连接
        kConnected,    // 已连接
        kDisconnected, // 关闭连接
    };

    void handleRead();
    void handleWrite();
    void handleClose(); // 关闭对socket的监听，并执行关闭回调(TcpServer::removeConnection)
    void handleError(); // 若遇到error，利用SO_ERROR套接字选项获取error值

    void setState(StateE s) { state_ = s; }

    StateE state_;
    EventLoop *loop_;
    std::string name_;
    std::unique_ptr<Socket> socket_; // TcpConnection持有socket_的目的是析构时自动close(sockfd)
    std::unique_ptr<Channel> channel_;
    InetAddress localAddr_; // 服务器本地的IP地址
    InetAddress peerAddr_;  // 对端客户的IP地址
    ConnectionCallback connCb_;
    MessageCallback messaCb_;
    CloseCallback closeCb_;
};