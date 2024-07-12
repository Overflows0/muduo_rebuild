#include <vector>
#include <memory>

#include "EventLoop.h"
#include "EventLoopThread.h"
#include "noncopyable.h"

/* 线程池 - 每个线程绑定EventLoop供每个TcpConnection循环利用，负责创建线程与循环并提供 */
class EventLoopThreadPool : noncopyable
{
public:
    EventLoopThreadPool(EventLoop *baseLoop);
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start();
    EventLoop *getNextLoop();

private:
    using EventLoopThreadPtrs = std::vector<std::unique_ptr<EventLoopThread>>;

    EventLoop *baseLoop_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<EventLoop *> loops_;
    EventLoopThreadPtrs threads_;
};