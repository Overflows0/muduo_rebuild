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
    void bindAddress(const InetAddress &address); // 封装bind()
    void listen();                                // 封装listen()
    int accept(InetAddress *peerAddress);         // 封装accept()并处理一些异常情况

    /* 剩下的静态函数是不必由独立对象调用，通过fd直接调用即可 */
    static int createNonblocking(); // 封装socket()创建非阻塞描述符
    static void shutdownWrite(int sockfd);
    static void close(int sockfd);
    static struct sockaddr_in getLocalAddr(int sockfd);
    static struct sockaddr_in getPeerAddr(int sockfd);
    static int getSocketError(int sockfd);
    static bool isSelfConnection(int sockfd);

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};