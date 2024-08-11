# muduo_rebuild项目

## 开发环境

* ubuntu 22.04
* g++ 11.4.0

## 功能介绍

1. Eventloop，ePollPoller（PollPoller）负责事件监听，acceptor负责处理连接请求。TcpServer的组成是Eventloop，epollPoler（PollPoller），acceptor，TimerQueue和ThreadPool，主线程（Reactor）监听连接请求，然后派发连接给工作线程（TcpConnection），处理IO事务；
2. Thread和EventLoop绑定组成EventloopThread，通过__thread标记的线程局部变量，实现one loop per thread的思想，使得多个EventLoop（逻辑单元）之间传递回调无锁，处理IO事务时保证线程安全；
3. 为LT模式非阻塞IO模型（epoll）设计的缓冲区Buffer，能够高并发地读写数据，内核是vector容器，对外表现为数据连续的自动扩容空间，保证足够数据有序性到达；

## 技术亮点

* 去除对Boost库依赖，改用C++11的库函数（目的是RAII，不必手动释放对象）：
使用unique_pointer，shared_pointer等智能指针；
使用C++11中的锁和线程库；
* 采用单reactor多线程模型，并且利用线程池技术，减少运行时创建线程开销，实现了高效并发的网络库；
* 回调函数队列为多个Eventloop的共享变量，采用swap()方式减少了锁的范围；
* 通过shared_pointer指针来管理处理IO事务的对象生命期，避免读写数据时异常中断连接；

## 安装使用

在项目文件夹当前目录下make，会生成测试用例ChatClient和ChatServer，其中Server需要指定端口号和线程参数，Client要指定端口号
执行服务器的例子：
`.\ChatServer 9985 3`
执行客户端的例子：
`.\ChatClient 9985`

### 模拟实时聊天室

假设服务器当前已建立多个TCP连接，每当有客户发来消息时，服务器会将该消息广播到每个客户机上（包括原客户机）；
