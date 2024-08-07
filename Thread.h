#pragma once

#include <thread>
#include <functional>

/**
 * C++11 thread的简单封装
 * 由于thread一被构造就创建线程，而为了在需要的时候才创建，所以进行封装
 */
class Thread
{
public:
    using threadFunction = std::function<void()>;

    Thread(const threadFunction &);
    ~Thread();
    void start();
    void join();

    bool started;
    bool joined;

private:
    threadFunction threadFunc;
    std::thread thread_;
};