#include <functional>
#include <memory>

class TcpConnection;
class Buffer;
class Timestamp;

// 提供全局的回调声明
using TimerCallback = std::function<void()>;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using MessageCallback = std::function<void(const TcpConnectionPtr &,
                                           Buffer *buf,
                                           Timestamp)>; // 该Timestamp是poll返回时刻，也就是接收到消息的那一刻
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t len)>;

class WeakCallback;