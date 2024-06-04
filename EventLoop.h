#include "vector"
#include "functional"

#include "Channel.h"
#include "Poller.h"

class EventLoop : noncopyable
{
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void stop();
    void updateChannel(Channel *channel_);
    void removeChannel(Channel *channel_);

private:
    Channel *channel_;
};