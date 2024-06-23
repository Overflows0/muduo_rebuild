#pragma once
#include <netinet/in.h>
#include <string>
#include <cstring>

/**
 * -对sockaddr_in简单封装
 * -将网络字节流转化为字符串
 */
class InetAddress
{
public:
    InetAddress(const sockaddr_in &addr)
        : addr_(addr) {}
    InetAddress(const uint16_t &port); // 只设置端口时，设置ip地址为通配符

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const sockaddr_in *getSockaddr() const { return &addr_; }
    void setSockaddr(const sockaddr_in &addr) { addr_ = addr; }

private:
    struct sockaddr_in addr_;
};