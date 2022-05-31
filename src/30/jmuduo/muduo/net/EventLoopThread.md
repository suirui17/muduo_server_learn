### EventLoopThread

任何线程只要创建并运行了EventLoop，都称之为I/O线程

I/O线程不一定是主线程

muduo并发模型 one loop per thread（每个I/O线程只有一个eventloop）+ threadpool（I/O线程池和计算线程池）

EventLoopThread封装了I/O线程

* 创建一个线程
 
* 在线程函数中创建一个EventLoop对象并调用EventLoop::loop

### EventLoopThread启动线程

#### EventLoopThread成员

* EventLoop
 
* Thread

* 条件变量Condition及锁MutexLock

#### EventLoopThread构造函数

* 创建一个Thread对象，绑定线程函数为EventLoopThread::threadFunc

* 创建并初始化锁及条件变量

* 设置线程初始化回调函数（如果不为空，则在loop函数之前被调用）

#### EventLoopThread::startLoop()

* → thread_.start()

* → 创建线程  pthread_create 绑定回调函数为startThread() 

* → thread->runInThread() 

* → func_ 即线程注册的回调函数，即EventLoopThread::threadFunc()

* → EventLoopThread::threadFunc()：创建一个eventloop并且将loop_指针指向该对象，开始loop循环

