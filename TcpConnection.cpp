#include "assert.h"

#include "TcpConnection.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Channel.h"
#include "Logger.h"

TcpConnection::TcpConnection(EventLoop *loop, std::string name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
    : loop_(loop),
      socket_(std::make_unique<Socket>(sockfd)),
      name_(name),
      state_(kConnecting),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      channel_(std::make_unique<Channel>(loop, sockfd))
{
    LOG_DEBUG("TcpConnection::ctor[%s] at %lu fd=%d", name_.c_str(), this, channel_->fd());
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG("TcpConnection::dtor[%s] at %lu fd=%d", name_.c_str(), this, channel_->fd());
}

void TcpConnection::connEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->enableReading();

    connCb_(shared_from_this());
}

void TcpConnection::connDestroyed()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnected);
    setState(kDisconnected);
    channel_->disableAll();
    connCb_(shared_from_this());

    loop_->removeChannel(channel_.get());
}

void TcpConnection::handleRead()
{
    char buffer[64812];
    ssize_t n = ::read(channel_->fd(), &buffer, sizeof(buffer));
    if (n > 0)
    {
        messaCb_(shared_from_this(), buffer, n);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        handleError();
    }
}

void TcpConnection::handleWrite()
{
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    assert(connected() == kConnected);
    channel_->disableAll();
    closeCb_(shared_from_this());
}

void TcpConnection::handleError()
{
    int err = Socket::getSocketError(channel_->fd());
    LOG_DEBUG("TcpConnection::handleError [%s] SO_ERROR = %d %s", name_, err, strerror_tl(err));
}