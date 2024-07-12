#pragma once

#include <memory>
#include <mutex>

#include "Callbacks.h"
#include "noncopyable.h"

class EventLoop;
class InetAddress;
class Connector;

/*
 *  TcpClient的功能需求：
 *  -把持Connector的生命期管理，负责TcpConnection的创建和关闭
 *  -遇到丢包或意外断开连接会反复尝试连接，延期连接时间随尝试次数增加而变长
 *  -只有一条TcpConnection连接，所以客户端的网络模型本质是单线程Reactor模型
 *  -意外或正常断开连接都能保证不会造成资源泄露的问题
 */
class TcpClient : noncopyable
{
public:
    TcpClient(EventLoop *loop, const InetAddress &serverAddr, const std::string nameArg);
    ~TcpClient();

    void connect();    // 开始向服务器发起连接
    void disconnect(); // 半关闭连接，待服务器知晓后就可以移除连接
    void stop();       // 注销定时器，停止尝试连接
    bool retry() const { return retry_; }
    void enableRetry() { retry_ = true; }

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connCb_ = std::move(cb);
    }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        wriComCb_ = std::move(cb);
    }
    void setMessageCallback(const MessageCallback &cb)
    {
        messaCb_ = std::move(cb);
    }

private:
    using ConnectorPtr = std::shared_ptr<Connector>;
    /* 成功建立连接时则创建TcpConnection */
    void newConnection(int sockfd);
    /* 关闭连接或尝试连接失败时需要移除TcpConnection */
    void removeConnection(const TcpConnectionPtr &conn);

    bool retry_;
    bool connect_;
    int nextConnId_;
    const std::string name_;
    EventLoop *loop_;
    TcpConnectionPtr connection_;
    ConnectorPtr connector_;
    MessageCallback messaCb_;
    WriteCompleteCallback wriComCb_;
    ConnectionCallback connCb_;
    std::mutex mutex_; // 这里使用锁是为了保证代码未来可能存在多线程环境而设计
};