#pragma once

#include <netinet/ip.h>

class InetAddress;

/* 封装服务器各自用于操作连接的接口(bind, listen, accept and close)，
 * 还有一些套接字选项(TCP_NODELAY,SO_REUSEADDR,SO_REUSEPORT,SO_KEEPALIVE)
 * 和获取连接错误与本地IP地址的函数
 */
class Socket
{
public:
    Socket(int sockfd)
        : sockfd_(sockfd)
    {
    }
    ~Socket();

    int fd() const { return sockfd_; }
    void bindAddress(const InetAddress &address);
    void listen();
    int accept(InetAddress *peerAddress);
    static void close(int sockfd);
    static struct sockaddr_in getLocalAddr(int sockfd);
    static int getSocketError(int sockfd);

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};