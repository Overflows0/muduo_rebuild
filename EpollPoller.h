#pragma once

#include <vector>
#include "Poller.h"

struct epoll_event;

/**
 *  -封装Epoll函数
**/

class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop *loop);
    ~EpollPoller() override;

    /**
     *  -传入需要监听的Channel列表，设置等待时间大小；
     *  -调用epoll_wait()，若监听到事件，则填充进活跃Channel列表当中；
     *  -对可能存在的一些异常进行处理；
     *  -返回监听到活跃描述符的时刻
     */
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    /* 更新维护Channel，添加、修改或删除对应描述符的监听事件 */
    void updateChannel(Channel *channel) override;
    /* 移除Channel并不会关闭描述符，只是移除在Poller的监听列表当中的记录（移除对Channel的监听），描述符的关闭由持有者析构时自动执行 */
    void removeChannel(Channel *channel) override;

private:
    using EventList = std::vector<struct epoll_event>;

    /* activeChannels是值-结果参数，填充活跃描述符 */
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    /* 封装对epoll_ctl()的调用 */
    void update(int operation, Channel *channel);
    const char *opeationToString(int op);

    static const int kInitEventListSize = 16;
    int epollfd_;
    EventList events_;     //监视多个epoll_event的列表
};