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
                                           Timestamp)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;