
#include "noncopyable.h"

class EventLoop;

class Channel : noncopyable
{
public:
    Channel(EventLoop *loop, int fd);
    ~Channel();

    void remove();
    void handleEvent();

private:
    void update();

    static const int kNonEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    EventLoop *loop_;
    int fd_;
    int events_;
    int revents_;
    int index_;
};