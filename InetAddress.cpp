#include <netinet/ip.h>
#include <string>
#include <arpa/inet.h>

#include "Logger.h"
#include "InetAddress.h"

InetAddress::InetAddress(const uint16_t &port)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = ::htonl(INADDR_ANY);
    addr_.sin_port = ::htons(port);
}

std::string InetAddress::toIp() const
{
    char buf[64];
    ::inet_ntop(AF_INET, &addr_.sin_addr.s_addr, buf, sizeof(addr_));
    return buf;
}

std::string InetAddress::toIpPort() const
{
    char buf[64];
    ::inet_ntop(AF_INET, &addr_.sin_addr.s_addr, buf, sizeof(addr_));
    ssize_t end = ::strlen(buf);
    uint16_t port = ::ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ::ntohs(addr_.sin_port);
}