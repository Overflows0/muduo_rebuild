#include "Thread.h"

Thread::Thread(const threadFunction &func)
    : started(false),
      joined(false)
{
    threadFunc = func;
}

Thread::~Thread()
{
    /* 没有被回收的running线程应当分离 */
    if (started && !joined)
    {
        thread_.detach();
    }
}

void Thread::start()
{
    started = true;
    thread_ = std::thread(threadFunc);
}

void Thread::join()
{
    joined = true;
    thread_.join();
}