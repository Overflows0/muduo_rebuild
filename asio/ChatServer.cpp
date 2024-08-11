#include "Codec.h"
#include "muduo_rebuild/TcpServer.h"
#include "muduo_rebuild/EventLoop.h"
#include "muduo_rebuild/TcpConnection.h"
#include "muduo_rebuild/InetAddress.h"

using namespace std::placeholders;

class ChatServer : noncopyable
{
public:
    ChatServer(EventLoop *loop, const InetAddress &listenAddr)
        : loop_(loop),
          server_(loop, listenAddr),
          codec_(std::bind(&ChatServer::onStringMessage, this, _1, _2, _3))
    {
        server_.setConnectionCallback(
            std::bind(&ChatServer::onConnection, this, _1));
        server_.setMessageCallback(
            std::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
    }
    void start()
    {
        server_.start();
    }
    void setThreadNum(int num)
    {
        server_.setThreadNum(num);
    }

private:
    void onConnection(const TcpConnectionPtr &conn)
    {
        LOG_INFO("%s -> %s is %s",
                 conn->localAddress().toIpPort().c_str(),
                 conn->peerAddress().toIpPort().c_str(),
                 conn->connected() ? "UP" : "DOWN");
        if (conn->connected())
        {
            connections_.insert(conn);
        }
        else
        {
            connections_.erase(conn);
        }
    }
    void onStringMessage(const TcpConnectionPtr &conn,
                         const std::string &message,
                         Timestamp)
    {
        for (const auto &it : connections_)
        {
            codec_.send(it, message);
        }
    }
    using ConnectionList = std::set<TcpConnectionPtr>;
    TcpServer server_;
    EventLoop *loop_;
    LengthHeaderCodec codec_;
    ConnectionList connections_;
};

int main(int argc, char *argv[])
{
    printf("pid = %d\n", getpid());
    if (argc > 2)
    {
        EventLoop loop;
        uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
        InetAddress listenAddr(port);
        ChatServer server(&loop, listenAddr);
        server.setThreadNum(atoi(argv[2]));
        server.start();
        loop.loop();
    }
    else if (argc > 1)
    {
        printf("Usage: %s %s threads of number\n", argv[0], argv[1]);
    }
    else
    {
        printf("Usage: %s port threads of number\n", argv[0]);
    }
}