### 服务器端启动流程

#### Channel：对I/O事件注册与响应的封装

* channel和fd是关联关系，一个channel一个fd，**channel不拥有文件描述符**，channel对象销毁时，不会关闭文件描述符，文件描述符为套接字所有，其生存期由套接字控制，即套接字销毁时关闭文件描述符

* channel和poller为聚合关系，一个poller可以捕捉多个channel的可读可写事件

* channel封装了**fd关注的事件**以及相应事件发生时的**回调函数**

* 部分数据成员

    * `int events_`：关注事件

    * `int revents_`：poll/epoll返回的事件

    * `int index_`：EpollPoller中为当前channel的状态 kNew/kAdded/kDeleted；PollPoller中为当前channel在poller的channel map中的索引

* poller/eventloop不负责**channel的生存期**，channel的生存期由TcpConnection、Acceptor、Connector负责，channel是以上对象的成员，也就是以上对象会将对I/O事件的注册和响应进行封装，并且注册到所属eventloop的poller中

* **`Channel::update() → EventLoop::updateChannel() → Poller::updateChannel()`**

#### Poller：I/O复用的抽象（实际上是对poll/epoll功能的封装）

* poller和eventloop是组合关系，一个eventloop一个poller，并且poller的生存期由eventloop来管理

* `EventList events_`：每个channel都有其对应的event，epoll会对事件进行管理（添加事件关注、修改事件关注、移除事件关注）

* `ChannelMap channels_`：EpollPoller维护了其channel map中每个channel的状态，同时负责对channel的添加、修改、删除

#### EventLoop：对事件循环的抽象

* EventLoop和线程绑定，每个线程最多一个EventLoop

* `EventLoop::loop()` → `Poller::poll()` 获取活跃的channel map；对每个活跃channel根据其活跃事件，调用事件处理函数Channel::handleEvent();`Channel::handleEvent()`根据到达的事件，回调相应的读/写/关闭/错误处理回调函数

* `wakeupChannel::ReadEventCallback`设置为`EventLoop::handleRead()`

* `boost::scoped_ptr<TimerQueue> timerQueue_`：定时器列表，定时器可以绑定回调函数，在定时器超时时刻调用回调函数

* `std::vector<Functor> pendingFunctors_`：I/O线程可进行部分非I/O处理

#### Acceptor：对被动套接字的抽象

* acceptor关注的是监听套接字的可读对象，由channel来注册

* 可读事件发生后，channel调用handleEvent，然后回调handleRead

* acceptor是TcpServer的成员，生存期由TcpServer负责

* 接收 eventloop* 和 InetAddress& 作为参数，创建非阻塞监听套接字和负责对该套接字关注I/O事件进行注册和相应的channel，设置channel的读回调函数`Acceptor::handleRead()`，即Acceptor实现了服务端连接接收器的实现逻辑，并将其注册为channel（连接到来I/O事件的抽象）的读回调函数

* `Acceptor::listen()`：监听套接字开始监听；将channel使能读事件（将读事件注册到eventloop对应的poller中

* `void Acceptor::handleRead()`：套接字accept一个客户端连接，成功后回调newConnectionCallback_ `TcpServer::newConnection()`；如果连接过多没有多余的描述符可用，则使用空闲文件描述符进行处理

#### Thread：创建线程，回调注册的线程函数

* `Thread::start()`：创建一个线程 → `Thread::startThread()` → `Thread::runInThread()` → `ThreadFunc()` 回调函数，线程实际执行的函数 `EventLoopThread::threadFunc`

* `Thread::join()`：等待当前线程执行结束

#### EventLoopThread：对eventloop和thread的封装

* 初始化时，eventloop为空；构造一个线程，但是没有启动；需要指定线程初始化回调函数（EventLoopThreadPool给出）

* 析构时，退出loop循环`loop_->quit()`，等待线程结束`thread_.join()`

* `EventLoopThread::startLoop()`：启动线程，并且使用信号量，等待eventloop创建完毕

* `EventLoopThread::threadFunc()`：创建eventloop，并且使用线程初始化回调函数，初始化eventloop所属线程；执行完毕后，通知`EventLoopThread::startLoop()`，该函数将创建的eventloop返回；`EventLoopThread::threadFunc()`在通知startloop之后，执行`loop.loop()`；

**startloop函数创建一个线程并且执行threadFunc函数，并且等待ThreadFunc函数创建eventloop，该事件发生后，startloop函数将创建eventloop对象返回，而ThreadFunc函数继续执行eventloop::loop()**

#### EventLoopThreadPool：对主线程和I/O线程的管理

* 主要负责对I/O线程的注册以及包括baseloop_在内所有线程的管理，EventLoopThreadPool不负责baseloop_的生存期，但是负责I/O线程的生存期

* `EventLoop* baseLoop_`：与acceptor所属的eventloop相同；该eventloop不是自己创建，需要TcpServer构造I/O线程池时指定

* I/O线程池

    * `boost::ptr_vector<EventLoopThread> threads_`：IO线程列表，当ptr_vector对象销毁时，它所管理的EventLoopThread对象也会跟着销毁

    * `std::vector<EventLoop*> loops_`：eventloop列表

    * 每个I/O线程都有自己对应的eventloop

* `EventLoopThreadPool::start(const ThreadInitCallback& cb)`：创建I/O线程并启动，需要给出线程初始化回调函数（TcpServer给出）；如果没有I/O线程，则需要使用线程初始化回调函数初始化baseloop_

* `EventLoopThreadPool::getNextLoop()`：采用轮叫的策略获取一个I/O线程分配任务


#### TcpConnection：对已连接套接字的抽象

* 部分数据成员

    * `boost::scoped_ptr<Socket> socket_`：成功连接的套接字

    * `boost::scoped_ptr<Channel> channel_`：套接字对应channel

    * `Buffer inputBuffer_`：应用层接收缓冲区

    * `Buffer outputBuffer_`：应用层发送缓冲区

    * 回调函数：连接建立/断开回调函数、消息到来回调函数、写完成回调函数、高水位标回调函数

* 构造函数：根据连接描述符新建一个连接套接字；根据连接套接字和eventloop新建一个channel；设置channel的读回调函数`TcpConnection::handleRead`、写回调函数`TcpConnection::handleWrite`、关闭连接回调函数`TcpConnection::handleClose`、错误处理回调函数`TcpConnection::handleError`

* `TcpConnection::send`：线程安全，可以跨线程调用 → `TcpConnection::sendInLoop`

* `TcpConnection::sendInLoop`：如果线程没有正在写入，并且发送缓冲区中没有数据，则直接写入内核缓冲区，对于大流量应用程序，需要设置writeCompleteCallback，如果数据没有全部写入内核缓冲区则需要将数据写入应用层发送缓冲区并且关注pollout事件（channel使能可写事件关注）

* `TcpConnection::shutdown()` → `TcpConnection::shutdownInLoop()` → `socket_->shutdownWrite()` 关闭写半端

* `TcpConnection::connectEstablished()`：设置TcpConnection状态；channel获得当前TcpConnection的弱引用，channel使能可读事件的关注，回调连接建立回调函数（TcpServer设定，应用程序给出）

* `TcpConnection::connectDestroyed()`：设置TcpConnection状态；取消channel关注的所有事件，调用连接断开回调函数；将channel从poller中移除

* `TcpConnection::handleRead(Timestamp receiveTime)`：将数据从内核缓冲区读取到应用层缓冲区，调用消息到来回调函数

* `TcpConnection::handleWrite()`：关注的pollout事件到达；将应用层发送缓冲区中的数据写入内核缓冲区，如果应用层发送缓冲区中没有数据，则取消关注pollout事件

* `TcpConnection::handleClose()`：取消channel关注事件，调用连接关闭回调函数`TcpServer::removeConnection(const TcpConnectionPtr& conn)` → `TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)` → `TcpConnection::connectDestroyed` → 连接断开回调函数（用户应用程序给出） 

#### TcpServer

* TcpServer和TcpConnection是聚合关系，一个TcpServer包含多个TcpConnection，但是不控制TcpConnection的生存期

* TcpServer和Acceptor是组合关系，TcpServer需要创建Acceptor，acceptor的创建需要指定eventloop和监听地址，该eventloop为TcpServer主线程对应eventloop，TcpsServer构造函数中，将Acceptor的连接建立回调函数注册为`TcpServer::newConnection`

* TcpServer和EventLoopThreadPool是组合的关系，TcpServer构造函数创建EventLoopThreadPool，需要给定acceptor的eventloop作为参数

* `TcpServer::start()`：启动线程池`threadPool_->start(threadInitCallback_)`线程初始化回调函数为用户程序给出；acceptor启动监听`loop_->runInLoop(Acceptor::listen)`

* `TcpServer::newConnection`：当套接字accept一个连接，成功后回调该函数
    
    * 从I/O线程池中取出一个线程作为ioloop

    * 使用ioloop、sockfd创建一个TcpConnection，设置Tcp连接的回调函数，ioloop运行`TcpConnection::connectEstablished`


* `TcpServer::removeConnection(const TcpConnectionPtr& conn)` → `TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)` → `TcpConnection::connectDestroyed` → 连接断开回调函数（用户应用程序给出）

* TcpServer和TcpConnection是聚合关系，Tcp生存期管理 ![](TcpConnection_lifetime.md)

#### Connector：对主动连接的抽象