### EventLoopThreadPool

* EventLoopThread：I/O线程类

* EventLoopThreadPool：I/O线程池类，开启多个I/O线程，并让这些I/O线程处于事件循环状态

![](multiple_reactors.png)

mainReactor关注监听套接字的事件

subReactors关注已连接套接字的事件

每当新到来一个连接，就选择一个subReactor处理该连接
