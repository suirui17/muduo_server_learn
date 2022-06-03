## Summary

### main

```c++
int main()
{
  LOG_INFO << "pid = " << getpid();
  muduo::net::EventLoop loop;
  muduo::net::InetAddress listenAddr(2007);
  EchoServer server(&loop, listenAddr);
  server.start();
  loop.loop();
}
```

### EchoServer

```c++
class EchoServer 
{
public:
    EchoServer(EventLoop* loop, const InetAddress& listenAddr);
    /* server_.setConnectionCallback(EchoServer::onConnection)
     * server_.setMessageCallback(EchoServer::onMessage)
     */ 
    void start();
    /* server_.satrt()
     */

private:
    /* 应用层连接到来/断开处理函数、消息到来处理函数
     */
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);

    EventLoop* loop;
    TcpServer server_; // 基于对象编程思想
};
```

### TcpServer

```c++
class TcpServer : boost::noncopyable
{
public:
    typedef boost::function<void(EventLoop*)> ThreadInitCallback;

    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg);
    /* acceptor_(new Acceptor(loop, listenAddr)),
     * threadpool_(new EventLoopThreadPool(loop)),
     * connectionCallback_(defaultConnectionCallback),
     * meaasgeCallback_(defaultMeaasgeCallback),
     * acceptor_.setNewConnectionCallback(TcpServer::newConnection);
     */
    ~TcpServer();
    /* 销毁线程池中的所有TcpConnection
     */

    void setThreadNum(int numThreads);
    void setThreadInitCallback(const ThreadInitCallback& cb);
    void setConnectionCallback(const ConnectionCallback& cb);
    void setMessageCallback(const MessageCallback& cb);
    void setWriteCompleteCallback(const WriteCompleteCallback& cb);

    void start();
    /* 如果当前TcpServer未启动，则设置started_为true
     *      threadPool_->start(threadInitCallback_);
     * 如果acceptor_未处于监听状态
     *      loop_->runInLoop(boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
     */

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpconnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    typedef std::map<string, TcpConnectionPtr> ConnectionMap;   
    // 连接名称和TcpConnection的映射

    EventLoop* loop；
    const string hostpot_; // 服务端口
    const string name_; // 服务名

    boost::scoped_ptr<Acceptor> acceptor_;
    boost::scoped_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;
    MessageCompleteCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallbakc threadInitCallback_; // IO线程池中的线程在进入事件循环之前会回调该函数

    bool started_;
    int nextConnId; // 下一个连接ID
    ConnectionMap connections_;

};
```

### Acceptor

```c++
class Accetor : boost::noncopyable
{
public:
    typedef boost::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr);
    /* 构造一个非阻塞的socket
     * 构造一个Channel，将其和Eventloop和socketfd绑定
     * 构造一个空闲文件描述符
     * 
     * acceptSocket_.setReuseAdder();
     * acceptSocket_.bindAddress(listenAddr); 绑定监听地址
     * acceptChannel_.setReadCallback(Acceptor::hanleRead()) 设置channel读回调函数
     */
    ~Acceptor();
    /* acceptChannel_disableAll(); 所有事件都设置为不关注
     * acceptChannel.remove() 将acceptChannel从eventloop的channel列表中移除
     * 关闭空闲文件描述符
     */

    void setNewConnectionCallback(const NewConnectionCallback& cb);

    void listen();
    /* 判断是否为eventloop所属线程调用
     * 状态设置为正在监听
     * acceptSocket_.listen(); 监听套接字
     * acceptChannel_.enableReading();  关注channel的可读事件
     */

private:
    void handleRead();
    /* 判断是否为eventloop所属线程调用
     * acceptSocket_.accept
     *      如果连接建立成功，且设置了连接建立回调函数，则回调newConnectionCallback_（TcpServer中设置的TcpServer::newConnectio）
     *      如果连接建立成功，但是没有设置回调函数，则关闭连接
     *      如果是连接过多错误，则关闭空闲描述符，接收客户端连接并立即断开，重新打开空闲文件描述符
     */

    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;

    NewConnectionCallback newConnectionCallback_;
    
    bool listening_; // 是否处于正在监听状态
    int idleFd_; // 空闲文件描述符，当连接过多时，接收客户端的连接并立即断开，然后重新打开空闲描述符
};
```

### EventLoopThreadPool

```c++
class EventLoopThreadPool : boost::noncopyable
{
public: 
    typedef boost::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop* baseloop);
    /* 使用baseloop初始化baseloop_
     */
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads);

    void start(const ThreadInitCallback& cb = ThreadInitCallback());
    /* 判断是否是baseloop_所属线程调用
     * started_置为true；
     * 每个I/O线程构造一个EventLoopThread(cb)，并且启动其对应的事件循环EventLoopThread::startLoop()
     * 如果没有I/O线程则使用线程初始化回调函数初始化baseloop_
     */

    EventLoop* getNextLoop();
    /* 采用轮叫的方式在I/O线程池中选择下一个EventLoop
     */

private:
    EventLoop* baseloop_; // 与Acceptor所属EventLoop相同
    bool started;
    int numTreads_; // I/O线程数
    int next_; // 新连接到来，所选择的EventLoop对象下标
    boost::ptr_vector<EventLoopThread> threads; // I/O线程列表
    std::vector<EventLoop*> loop_; // EventLoop列表
};
```

### EventLoopThread

```c++
class EventLoopThread : boost::noncopyable
{
public:
    typedef boost::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback& cb = TreadInitCallback());
    /* loop_ 初始化为NULL
     * 构造Thread，线程函数绑定为EventLoopThread::threadFunc
     * 初始化锁和条件变量
     * 设置线程初始化回调函数
     */
    ~EventLoopThread();
    /* 退出I/O线程 loop_quit();
     * thrad_.join(); 等待线程退出
     */

    Eventloop* startLoop();
    /* 如果线程未启动
     *      thread_start();
     * 等待EventLoop创建
     * 返回EventLoop*
     */

private:
    void threadFunc(); // 线程函数
    /* 创建eventloop，如果设置了线程初始化回调函数，则callback_(&loop)
     * 通知正在等待的startLoop函数
     * loop.loop();
     */

    EventLoop* loop_;
    bool exiting_;

    Thread thread_;
    MutexLock mutex_;
    Condition cond_;

    ThreadInitCallback callback_;
};
```

### Thread

```c++
class Thread : boost::noncopyable
{
public:
    typedef boost::function<void()> ThreadFunc;

    explicit Thread(const ThreadFunc&, const string& name = string());
    ~Thread();

    void start();
    /* 创建线程 → startThread() → runInLoop() → ThreadFunc() 
     */

    int join();
    /* 等待当前线程运行结束
     */

private:
    static void* startThread(void* thread);
    void runInThread();

    bool started_; // 线程是否已经启动
    pthread_t pthreadId_; // 进程独立id空间内的线程id
    pid_t tid_; // 线程真实id
    ThreadFunc func_; 
    string name_;

    static AtomicInt32 numCreated_; // 静态成员，原子操作，每构建一个线程就+1
};
```

### Socket

```c++
class Socket : boost::noncopyable
{
public:
    explicit Socket(int sockfd) : sockfd_(sockfd){}
    ~Socket();

    void bindAddress(const InetAddress& localaddr);
    /* ::bind
     */
    void listen();
    /* ::listen
     */

    int accept(InetAddress* peeraddr);
    /* ::accept
     */

    void shutdownWrite();
    /* ::shutdown
     */

    // 是否禁用Nagle算法
    void setTcpNoDelay(bool on);

    // 是否设置地址重用
    void setReuseAddr(bool on);

    // 探测连接是否存在
    void setKeepAlive(bool on)

private:
    const int sockfd_;
};
```

### Channel

```c++
class Channel : boost::noncopyable
{
public:
    typedef boost::function<void()> EventCallbacl;
    typedef boost::function<void(Timestamp)> ReadEventCallback;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);
    /* 如果绑定了TcpConnection，则将弱引用提升
     * handleEventguard(receiveTime);
     */
    void setReadCallback(const ReadEventCallback& cb);
    void setWriteCallback(const EventCallback& cb);
    void setCloseCallback(const EventCallback& cb);
    void setErrorCallback(const EventCallback& cb);

    void tie(const boost::shared_ptr<void>&)

    void enableReading() { events_ |= kReadEvent; update(); }
    // void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    bool isWriting() const { return events_ & kWriteEvent; }

    void remove();
private:
    void update();
    /* loop_->updateChannel();
     */
    void handleEventWithGuard(Timestamp receiveTime);
    /* 设置为事件处理状态，对不同事件调用相应的回调函数
     */

    static const int kNoneEvent; // 0
    static const int kReadEvent; // POLLIN | POLLPRI
    static const int kWriteEvent; // POLLOUT

    EventLoop* loop_;
    const int fd_; // 文件描述符，channel不负责对其进行关闭
    int events_; // 关注的事件
    int revents_; // poll/epoll返回的事件
    int index_; // 在poller的channel数组中索引
    bool logHup_;

    boost::weak_ptr<void> tie_;
    bool tied_;
    bool eventHandling; // 是否处于事件处理中

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
```

### CurrentThread

```c++
namespace CurrentThread
{
  // __thread修饰的变量是线程局部存储的。
  __thread int t_cachedTid = 0;		// 线程真实pid（tid）的缓存，
									// 是为了减少::syscall(SYS_gettid)系统调用的次数
									// 提高获取tid的效率
  __thread char t_tidString[32];	// 这是tid的字符串表示形式
  __thread const char* t_threadName = "unknown";
  const bool sameType = boost::is_same<int, pid_t>::value;
  BOOST_STATIC_ASSERT(sameType);
}
// 定义了线程独有的数据成员
```

### EventLoop

```c++
class EventLoop : boost::noncopyable
{
public:
    typedef boost::function<void()> Functor;

    EventLoop();
    /* 构造一个默认poller
     * 构造timerQueue
     * 创建一个eventfd作为wakeupfd
     * 创建wakeupChannel和wakeupfd绑定
     * wakeupChannel_->setReadCallback(boost::bind(&EventLoop::handleRead, this))；
     * wakeupChannel_->enableReading();
     */
    ~EventLoop();
    /* 关闭wakeupfd
     */

    void loop();

    void quit();

    void runInLoop(const Functor& cb);

    void queueInLoop(const Functor& cb);

    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    TimerId runAfter(double delay, const TimerCallback& cb);
    TimerId runEvery(double interval, const TimerCallback& cb);

    void cancel(TimerId timerId);

    void wakeup();
    void updateChannel(Channel* channel);		// 在Poller中添加或者更新通道
    /* poller_->updateChannel(channel);
     */
    void removeChannel(Channel* channel);		// 从Poller中移除通道

private:
    void handleRead();
    void doPendingFunctors();

    typedef std::vector<Channel*> ChannelList;

    bool looping_;
    bool quit_;
    bool eventHandling_;
    bool callingPendingFunctors_;

    const pid_t threadId;
    Timestamp pollReturnTime;

    boost::scoped_ptr<Poller> poller_;
    boost::scoped_ptr<TimerQueue> timerQueue_;

    int wakeFd_;
    boost::scoped_ptr<Channel> wakeupChannel_; // 该通道也会被纳入poller_中管理

    ChannelList activeChannels_; // Poller返回的活动通道
    Channel* currentActiveChannel_; // 当前正在处理的活动通道

    MutexLock mutex_;
    std::vector<Functor> pendingFunctors_;
};
```

### Epoller

```c++
class EPoller : public Poller
{
public:
    EpollPoller(EventLoop* loop);
    /* epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
     */
    vitual ~EpollPoller();
    /* ::close(epollfd_);
     */

    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
    /* ::epoll_wait 监听事件
     * fillActiveChannels(numEvents, activeChannels); 返回活跃通道
     */
    virtual void updateChannel(Channel* channel);
    /* 添加channel、删除channel、修改channel
     */
    virtual void removeChannel(Channel* channel);
    /* 将channel的fd从channels列表中删除
     * 设置epoll不再关注channel
     */

private:
    static const int kInitEventListSize = 16;
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channne* channel);
    /* 根据调用者给定的操作对poller关注的channel进行修改
     */

    typedef std::vector<struct epoll_event> EventList;
    typedef std::map<int, Channel*> ChannelMap;

    int epollfd_;
    EventList events_;
    ChannelMap channels_;
};
```