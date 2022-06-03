### 服务器端启动流程

#### Channel：对I/O事件注册与响应的封装

* channel和fd是关联关系，一个channel一个fd，**channel不拥有文件描述符**，channel对象销毁时，不会关闭文件描述符，文件描述符为套接字所有，其生存期由套接字控制，即套接字销毁时关闭文件描述符

* channel和poller为聚合关系，一个poller可以捕捉多个channel的可读可写事件

* channel封装了**fd关注的事件**以及相应事件发生时的**回调函数**

```c++
int events_; // 关注事件
int revents_; // poll/epoll返回的事件
int index_; // EpollPoller中为当前channel的状态 kNew/kAdded/kDeleted
            // PollPoller中为当前channel在poller的channel map中的索引
```

* poller/eventloop不负责**channel的生存期**，channel的生存期由TcpConnection、Acceptor、Connector负责，channel是以上对象的成员，也就是以上对象会将对I/O事件的注册和响应进行封装，并且注册到所属eventloop的poller中

* Channel::update() → EventLoop::updateChannel() → Poller::updateChannel()

#### Poller：I/O复用的抽象（实际上是对poll/epoll功能的封装）

* poller和eventloop是组合关系，一个eventloop一个poller，并且poller的生存期由eventloop来管理

* EventLoop::loop() → Poller::poll()

* EventList events_：每个channel都有其对应的event，epoll会对事件进行管理（添加事件关注、修改事件关注、移除事件关注）

* ChannelMap channels_：EpollPoller维护了其channel map中每个channel的状态，同时负责对channel的添加、修改、删除

#### EventLoop：对事件循环的抽象

* EventLoop和线程绑定，每个线程最多一个EventLoop，<font color=red>内容</font>


