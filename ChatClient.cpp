#include <functional>
#include <string>
#include <mutex>
#include <iostream>

#include "Codec.h"
#include "EventLoopThread.h"
#include "TcpClient.h"
#include "InetAddress.h"
#include "Logger.h"

using namespace std::placeholders;

class ChatClient : noncopyable
{
public:
    ChatClient(EventLoop *loop, const InetAddress &serverAddr)
        : loop_(loop),
          client_(loop, serverAddr, "ChatClient"),
          connected_(false),
          codec_(std::bind(&ChatClient::onStringMessage, this, _1, _2, _3))
    {
        client_.setConnectionCallback(std::bind(&ChatClient::onConnection, this, _1));
        client_.setMessageCallback(std::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
    }
    void connect()
    {
        client_.connect();
    }
    void disconnect()
    {
        client_.disconnect();
    }
    bool connected() const
    {
        return connected_;
    }

    void write(const std::string &message)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (connection_)
        {
            codec_.send(connection_, message);
        }
    }

private:
    using StringMeassageCallback = std::function<void(const TcpConnectionPtr &conn,
                                                      const std::string &message,
                                                      Timestamp receiveTime)>;
    void onStringMessage(const TcpConnectionPtr &conn,
                         const std::string &message,
                         Timestamp receiveTime)
    {
        printf("<<< %s\n", message.c_str());
    }
    void onConnection(const TcpConnectionPtr &conn)
    {
        LOG_INFO("%s -> %s is %s",
                 conn->localAddress().toIpPort().c_str(),
                 conn->peerAddress().toIpPort().c_str(),
                 conn->connected() ? "UP" : "DOWN");
        std::lock_guard<std::mutex> lk(mutex_);
        if (conn->connected())
        {
            connection_ = conn;
            connected_ = true;
        }
        else
        {
            connection_.reset();
            connected_ = false;
        }
    }
    EventLoop *loop_;
    TcpClient client_;
    LengthHeaderCodec codec_;
    std::mutex mutex_;
    TcpConnectionPtr connection_;
    bool connected_;
};

int main(int argc, char **argv)
{
    printf("pid = %d\n", getpid());
    if (argc > 1)
    {
        EventLoopThread loopThread;
        uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
        InetAddress serverAddr(port);
        ChatClient client(loopThread.startLoop(), serverAddr);
        client.connect();
        std::string line;
        while (std::getline(std::cin, line) && client.connected())
        {
            client.write(line);
        }
        client.disconnect();
    }
    else
    {
        printf("Usage: %s port", argv[0]);
    }
}