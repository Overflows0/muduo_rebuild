#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "Socket.h"
#include "InetAddress.h"
#include "Logger.h"

Socket::~Socket()
{
    Socket::close(sockfd_);
}

int Socket::createNonblocking()
{
    int sockfd = ::socket(AF_INET,
                          SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                          IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_ERROR("socket() failed");
    }
    return sockfd;
}

void Socket::bindAddress(const InetAddress &address)
{
    if (::bind(sockfd_, (sockaddr *)address.getSockaddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind() sockfd%d failed", sockfd_);
    }
}

void Socket::listen()
{
    if (::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen() sockfd%d failed", sockfd_);
    }
}

int Socket::accept(InetAddress *peerAddress)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);
    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        peerAddress->setSockaddr(addr);
    }
    else
    {
        int savedErrno = errno;
        LOG_FATAL("Socket::accept");
        switch (savedErrno)
        {
        case EAGAIN:
        case ECONNABORTED:
        case EINTR:
        case EPROTO:
        case EPERM:
        case EMFILE:
            errno = savedErrno;
            break;
        case EBADF:
        case EFAULT:
        case EINVAL:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:
            LOG_FATAL("unexpected error of acccept %d", savedErrno);
            break;
        default:
            LOG_FATAL("unknown error of accept %d", savedErrno);
            break;
        }
    }
    return connfd;
}

void Socket::shutdownWrite(int sockfd)
{
    if (::shutdown(sockfd, SHUT_WR) < 0)
    {
        LOG_DEBUG("Socket::shutdown");
    }
}

void Socket::close(int sockfd)
{
    if (::close(sockfd))
    {
        LOG_INFO("Socket::close");
    }
}

sockaddr_in Socket::getLocalAddr(int sockfd)
{
    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    socklen_t len = sizeof(localAddr);
    if (::getsockname(sockfd, (sockaddr *)&localAddr, &len) < 0)
    {
        LOG_ERROR("Socket::getLocalAddr");
    }
    return localAddr;
}

sockaddr_in Socket::getPeerAddr(int sockfd)
{
    struct sockaddr_in peerAddr;
    socklen_t len = sizeof(peerAddr);
    memset(&peerAddr, 0, sizeof(peerAddr));
    if (::getpeername(sockfd, (sockaddr *)&peerAddr, &len) < 0)
    {
        LOG_ERROR("Socket::getPeerAddr");
    }
    return peerAddr;
}

bool Socket::isSelfConnection(int sockfd)
{
    struct sockaddr_in localAddr = Socket::getLocalAddr(sockfd);
    struct sockaddr_in peerAddr = Socket::getPeerAddr(sockfd);
    return localAddr.sin_addr.s_addr == peerAddr.sin_addr.s_addr && localAddr.sin_port == peerAddr.sin_port;
}

int Socket::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = sizeof(optval);
    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}