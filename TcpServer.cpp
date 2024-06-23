#include "assert.h"

#include "TcpServer.h"
#include "InetAddress.h"
#include "Acceptor.h"
#include "Logger.h"
#include "EventLoopThread.h"
#include "TcpConnection.h"

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr)
    : loop_(loop),
      started_(false),
      name_(listenAddr.toIpPort()),
      acceptor_(std::make_unique<Acceptor>(loop, listenAddr)),
      nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    started_ = false;
    for (auto &it : connections_)
    {
        it.second.reset();
    }
}

void TcpServer::start()
{
    if (!started_)
    {
        started_ = true;
    }
    if (!acceptor_->listenning())
    {
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    loop_->assertInLoopThread();
    char buf[64];
    snprintf(buf, sizeof(buf), "#%d", nextConnId_);
    nextConnId_++;
    std::string name = name_ + buf;

    LOG_INFO("TcpServer::newConnection[%s] new connection[%s] from %s", name_.c_str(), name.c_str(), peerAddr.toIpPort().c_str());
    InetAddress localAddr(Socket::getLocalAddr(sockfd));
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, name, sockfd, localAddr, peerAddr);
    connections_[name] = conn;

    conn->setConnectionCallback(connCb_);
    conn->setMessageCallback(messaCb_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    conn->connEstablished();
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    LOG_INFO("TcpServer::removeConnection [%s] - connection %s", name_.c_str(), conn->name().c_str());
    ssize_t n = connections_.erase(conn->name());
    assert(n == 1);
    (void)n;
    loop_->queueInLoop(std::bind(&TcpConnection::connDestroyed, conn));
}