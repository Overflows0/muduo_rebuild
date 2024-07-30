#pragma once

#include <functional>
#include <arpa/inet.h>
#include "TcpConnection.h"
#include "Buffer.h"
#include "noncopyable.h"
#include "Timestamp.h"
#include "Logger.h"

class LengthHeaderCodec : noncopyable
{
public:
    using StringMessageCallback =
        std::function<void(const TcpConnectionPtr &conn,
                           const std::string &message,
                           Timestamp)>;
    explicit LengthHeaderCodec(const StringMessageCallback &cb)
        : messageCallback_(cb)
    {
    }

    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buffer,
                   Timestamp receiveTime)
    {
        while (buffer->readableBytes() >= kHeaderLen)
        {
            const void *data = buffer->peek();
            const int32_t be32 = *static_cast<const int32_t *>(data);
            const int32_t len = ntohl(be32);
            if (len > 65536 || len < 0)
            {
                LOG_ERROR("Invalid length: %d", len);
                conn->shutdown();
                break;
            }
            else if (buffer->readableBytes() >= len + kHeaderLen)
            {
                buffer->retrieve(kHeaderLen);
                std::string message(buffer->peek(), len);
                messageCallback_(conn, message, receiveTime);
                buffer->retrieve(len);
            }
            else
                break;
        }
    }

    void send(const TcpConnectionPtr &conn, const std::string &message)
    {
        Buffer buffer;
        buffer.append(message);
        int32_t len = static_cast<int32_t>(message.size());
        int32_t be32 = htonl(len);
        buffer.prepend(&be32, sizeof(be32));
        conn->send(&buffer);
    }

private:
    StringMessageCallback messageCallback_;
    const static size_t kHeaderLen = sizeof(int32_t);
};