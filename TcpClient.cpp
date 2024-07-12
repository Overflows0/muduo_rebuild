#include "Logger.h"
#include "Connector.h"
#include "TcpConnection.h"
#include "InetAddress.h"
#include "TcpClient.h"
#include "EventLoop.h"

using namespace std::placeholders;

/* 只为析构时调用，此时不必再尝试连接 */
void removeConnection_base(EventLoop *loop, const TcpConnectionPtr &conn)
{
    loop->queueInLoop(std::bind(
        &TcpConnection::connDestroyed, conn));
}

TcpClient::TcpClient(EventLoop *loop, const InetAddress &serverAddr, const std::string nameArg = "")
    : loop_(loop),
      connector_(std::make_shared<Connector>(loop, serverAddr)),
      retry_(false),
      connect_(true),
      name_(nameArg),
      nextConnId_(1)
{
    connector_->setNewConnectionCallback(std::bind(
        &TcpClient::newConnection, this, std::placeholders::_1));
    LOG_INFO("TcpClient::ctor - TcpClient[%s]'s connector %zd",
             name_.c_str(), connector_.get());
}

TcpClient::~TcpClient()
{
    LOG_INFO("TcpClient::dtor - TcpClient[%s]'s connector %zd",
             name_.c_str(), connector_.get());
    bool unique;
    TcpConnectionPtr conn; // 即使是TcpClient先被析构，也能留下一个共享指针计数，保证连接能够关闭
    {
        std::lock_guard<std::mutex> lk(mutex_);
        unique = connection_.unique();
        conn = connection_;
    }
    /* 如果有连接存在，且计数为1(说明只有TcpClient持有)，立即关闭 */
    if (conn)
    {
        loop_->runInLoop(std::bind(
            &TcpConnection::setCloseCallback, conn,
            CloseCallback(std::bind(&removeConnection_base, loop_, _1))));
        if (unique)
        {
            conn->forceClose();
        }
    }
}

void TcpClient::connect()
{
    LOG_INFO("TcpClient::connect[%s] connecting to %s",
             name_.c_str(), connector_->severAddress().toIpPort().c_str());
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect()
{
    connect_ = false;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
    // 此时已经与服务器成功建立全双工通道
    loop_->assertInLoopThread();

    /* 建立TcpConnection */
    InetAddress peerAddr(Socket::getPeerAddr(sockfd));
    InetAddress localAddr(Socket::getLocalAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof(buf), ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    TcpConnectionPtr conn(std::make_shared<TcpConnection>(loop_,
                                                          connName,
                                                          sockfd,
                                                          localAddr,
                                                          peerAddr));
    conn->setConnectionCallback(connCb_);
    conn->setMessageCallback(messaCb_);
    conn->setCloseCallback(std::bind(
        &TcpClient::removeConnection, this, _1));

    /* 共享TcpConnection的所有权 */
    {
        std::lock_guard<std::mutex> lk(mutex_);
        connection_ = conn;
    }

    conn->connEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    /* 释放客户端对TcpConnection所有权 */
    {
        std::lock_guard<std::mutex> lk(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    /* 连接可能还在执行IO事务中，延后释放TcpConnection资源 */
    loop_->queueInLoop(std::bind(&TcpConnection::connDestroyed, conn));

    if (retry_ && connect_)
    {
        LOG_INFO("TcpClient::connect[%s] - Reconnecting to %s",
                 name_.c_str(),
                 conn->peerAddress().toIpPort().c_str());
        connector_->restart();
    }
}