#pragma once

#include <memory>
#include <functional>

#include "InetAddress.h"
#include "TimerId.h"
#include "Callbacks.h"
#include "noncopyable.h"

class Channel;
class EventLoop;

/* Connector 功能：
 * -封装::connect()，使其具有反复尝试连接的行为
 * -每次尝试连接都会注销Channel关闭套接字，再重新注册Channel创建套接字
 * -每次尝试连接的时机由定时器把控，Connector析构前会注销定时器
 */
class Connector : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd)>;
    Connector(EventLoop *loop, const InetAddress &serverAddr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = std::move(cb);
    }
    /* 其他对象都能调用此接口去启动::connect() */
    void start();
    /* 重置延期时间，然后重新启动::connect() */
    void restart();
    /* 注销定时器 */
    void stop();

    const InetAddress &severAddress() const { return serverAddr_; }

private:
    enum State
    {
        kDisconnected, // 未连接，调用::connect()前的状态
        kConnecting,   // 连接中，调用::connect()后，但有可能连接失败
        kConnected     // 已连接，准备执行建立TcpConnection的回调
    };
    static const int kMaxRetryDelayMs = 30 * 1000;
    static const int kInitRetryDelayMs = 500;

    void startInLoop(); // 由独占的EventLoop去调用::connect()
    void setState(State state) { state_ = state; }
    void handleWrite(); // 套接字可写并不意味着成功连接，还需要处理潜在错误
    void handleError(); // 处理连接失败的异常情况，重新尝试连接时会移除channel再retry()
    void connect();     // 封装::conect()，以便具备反复连接和处理各种错误的功能
    void connecting(int sockfd);
    void retry(int sockfd); // 关闭套接字，并定期反复尝试连接
    void resetChannel();
    int removeAndResetChannel();

    EventLoop *loop_;
    InetAddress serverAddr_;
    NewConnectionCallback newConnectionCallback_;
    State state_;  // 该变量表示客户端实际的连接状态
    bool connect_; // 该变量表示客户端期望的连接状态
    int retryDelayMs_;
    std::unique_ptr<Channel> channel_;
    TimerId timerId_;
};