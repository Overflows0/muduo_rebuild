#include <assert.h>

#include "Connector.h"
#include "Logger.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"

const int Connector::kInitRetryDelayMs;
const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs)
{
}

Connector::~Connector()
{
    loop_->cancel(timerId_);
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);

    if (connect_)
    {
        connect();
    }
    else
    {
        LOG_DEBUG("do not connect");
    }
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::stop()
{
    connect_ = false;
    loop_->cancel(timerId_);
}

void Connector::connect()
{
    int sockfd = Socket::createNonblocking();
    const struct sockaddr *addr = serverAddr_.getSockaddr();
    int ret = ::connect(sockfd,
                        addr,
                        static_cast<socklen_t>(sizeof(struct sockaddr_in)));
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS: // 正在连接中
    case EINTR:       // 被系统调用中断
    case EISCONN:     // 正在连接中
        connecting(sockfd);
        break;

    case EAGAIN: // 本机的瞬息端口暂时用完，需要延后重试
    case EADDRINUSE:
    case EADDRNOTAVAIL: // 请求地址不可用，可能是网络连接超时，可以尝试再次连接
    case ECONNREFUSED:  // 服务器当时拒绝连接，可能稍后开始接受连接，可以尝试再次连接
    case ENETUNREACH:   // 网络当时时刻不可达，可以尝试重新连接
        retry(sockfd);
        break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
        LOG_ERROR("connect error in Connector::startInloop ");
        Socket::close(sockfd);
        break;

    default:
        LOG_ERROR("Unexpected error in Connector::startInLoop ");
        Socket::close(sockfd);
        break;
    }
}

void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(std::bind(&Connector::handleWrite, this));
    channel_->setErrorCallback(std::bind(&Connector::handleError, this));
    channel_->enableWriting();
}

void Connector::resetChannel()
{
    channel_.reset();
}

int Connector::removeAndResetChannel()
{
    int sockfd = channel_->fd();
    channel_->disableAll();
    loop_->removeChannel(channel_.get());
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
    return sockfd;
}

void Connector::retry(int sockfd)
{
    Socket::close(sockfd);
    setState(kDisconnected);
    if (connect_)
    {
        LOG_INFO("Connector::retry - Retrying connecting to %s in %d millseconds",
                 serverAddr_.toIpPort().c_str(),
                 retryDelayMs_);
        timerId_ = loop_->runAfter(retryDelayMs_ / 1000.0, std::bind(&Connector::connect, this));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }
}

void Connector::handleWrite()
{
    LOG_DEBUG("Connector::handleWrite");

    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = Socket::getSocketError(sockfd);
        /* 处理其他可能存在的错误*/
        if (err)
        {
            LOG_DEBUG("Connector::handleWrite - SO_ERROR %d: %s", err, strerror_l(err));
        }
        /* 即使能连接，也有可能是自连接（连接到本地主机IP地址和侦听端口上）*/
        else if (Socket::isSelfConnection(sockfd))
        {
            LOG_DEBUG("Connector::handleWrite - Self-Connection");
            retry(sockfd);
        }
        /* 成功连接则执行建立TcpConnection的回调 */
        else
        {
            setState(kConnected);
            if (connect_)
            {
                newConnectionCallback_(sockfd);
            }
            /* 意味中断连接，需要关闭套接字 */
            else
            {
                Socket::close(sockfd);
            }
        }
    }
    else
    {
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError()
{
    LOG_ERROR("Connect::handleError");

    /* 如果是依然处于连接中，则需要重新连接 */
    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = Socket::getSocketError(sockfd);

        LOG_DEBUG("SO_ERROR %d: %s", err, strerror_l(err));
        retry(sockfd);
    }
}