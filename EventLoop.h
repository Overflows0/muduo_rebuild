#include "vector"
#include "functional"

#include "CurrentThread.h"

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    EventLoop *getEventLoopOfCurrentThread();

    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInThread();
        }
    }

    bool isInLoopThread() const { return CurrentThread::tid() == threadId_; }

private:
    void abortNotInThread();
    bool looping_;
    const pid_t threadId_;
};