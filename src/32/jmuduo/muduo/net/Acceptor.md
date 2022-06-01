### Acceptor

Acceptor数据成员包括

* Socket：监听套接字（服务器端套接字）

* Channel：用于观察此socket的可读事件，channel调用handleEvent()，然后回调Acceptor::handleRead()，进而调用accept来接受新的连接，并回调用户callback

### 相关函数

```c++
int accept(int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen);
```

第一个参数为监听套接字

cliaddr和addrlen用来返回已连接的对端进程的协议地址，均为值-结果参数

该函数的返回值是内核自动生成的一个全新描述符，代表与所返回客户的TCP连接

### Acceptor处理流程

* 创建一个协议地址，可以只指定端口，也可以指定IP地址和端口

* 创建一个EventLoop对象

* 创建Acceptor(EventLoop, 监听协议地址) 

    * 绑定EventLoop

    * 创建socket，将其绑定监听地址

    * 创建channel(EventLoop, socket的文件描述符)

    * 创建空闲文件描述符

    * 将channel的读回调函数设置为Acceptor::handleRead（Acceptor包含了一个acceptSocket和一个acceptChannel，channel属于一个EventLoop，channel会有读、写、关闭、错误四种回调函数）

* 设置连接回调函数

* 监听Acceptor

    * Socket::listen() → socket::listenOrDie() → ::listen()

    * Channel::enableReading() → Channel::update() → EventLoop::updateChannel() → Poller::updateChannel() channel关注文件描述符的可读事件，并且将channel注册至Poller中

* socket::accept，将对端协议地址通过值-结果参数的形式返回，同时返回一个全新的描述符，表示和对端的TCP连接

* eventloop.loop()实际上调用poll，监听关注的事件