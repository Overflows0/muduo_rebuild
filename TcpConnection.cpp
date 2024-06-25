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
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG("TcpConnection::dtor[%s] at %lu fd=%d", name_.c_str(), this, channel_->fd());
}

void TcpConnection::send(const void *message, size_t len)
{
    send(std::string(static_cast<const char *>(message), len));
}

void TcpConnection::send(const std::string &message)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, message));
        }
    }
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
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
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    channel_->disableAll();
    connCb_(shared_from_this());

    loop_->removeChannel(channel_.get());
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    /* 正常接收到消息就执行消息回调 */
    {
        messaCb_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    /* 读到0字节就执行关闭回调关闭连接 */
    {
        handleClose();
    }
    else
    /* 异常读入就执行异常回调 */
    {
        errno = savedErrno;
        LOG_DEBUG("TcpConnection:handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    /* 如果套接字可写，就执行写，否则（触发写事件却无监听）就说明连接关闭 */
    if (channel_->isWriting())
    {
        ssize_t n = ::write(channel_->fd(),
                            outputBuffer_.peek(),
                            outputBuffer_.readableBtyes());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            /* 如果输出缓冲区全部发送完毕就关闭写监听 */
            if (outputBuffer_.readableBtyes() == 0)
            {
                channel_->disableWriting();
                /* 如果连接处于半关闭状态就开始关闭对套接字的写方向 */
                if (state_ == kDisconnecting)
                    shutdownInLoop();
            }
            else
            {
                LOG_DEBUG("I am going to write more data");
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_DEBUG("Connection is down, no more writing");
    }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnected || state_ == kDisconnecting);
    channel_->disableAll();
    closeCb_(shared_from_this()); //<-TcpServer::removeConnection
}

void TcpConnection::handleError()
{
    int err = Socket::getSocketError(channel_->fd());
    LOG_DEBUG("TcpConnection::handleError [%s] SO_ERROR = %d %s", name_, err, strerror_tl(err));
}

void TcpConnection::sendInLoop(const std::string &message)
{
    loop_->assertInLoopThread();
    ssize_t n = 0;
    /* 先将数据直接发送到套接字中，条件是套接字没有正在写，缓冲区为空 */
    if (!channel_->isWriting() && outputBuffer_.readableBtyes() == 0)
    {
        n = ::write(channel_->fd(), message.data(), message.size());
        if (n < 0)
        {
            n = 0;
            if (errno != EWOULDBLOCK)
                LOG_DEBUG("TcpConnection::sendInLoop");
        }
        else
        {
            if (static_cast<size_t>(n) < message.size())
            {
                LOG_DEBUG("I am going to write more data");
            }
        }
    }

    assert(n >= 0);
    /*
     *  如果不能一次性发送完全部消息，或者缓冲区已经有数据或套接字正在写，
     *  就先待发送数据存储在缓冲区中，注册写监听(假如没有正在写)
     */
    if (static_cast<size_t>(n) < message.size())
    {
        outputBuffer_.append(message.data() + n, message.size() - n);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting())
    {
        Socket::shutdownWrite(channel_->fd()); // 半关闭写入
    }
}