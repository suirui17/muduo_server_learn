### WriteCompleteCallback

数据发送完毕回调函数，即所有的用户数据都已拷贝到内核缓冲区时回调该函数

通知上层应用程序可以发送更多数据

通常只有编写大流量应用程序才需要关注writeCompleteCallback,低流量应用程序不需要关注，原因：

* 大流量应用程序：不断产生数据，然后调用conn->send()；如果对等方接收不及时，收到通告窗口的控制，内核发送缓冲不足，会将用户数据添加到应用层发送缓冲区；发送缓冲区内存可能会不断扩大

* 解决方法：调整发送频率，关注WriteCompleteCallback，当应用层发送缓冲区中的数据全部拷贝到内核之后，上层应用程序通过WriteCompleteCallback得到通知，再发送应用数据

outputBuffer_被清空也会回调该函数，可以理解为低水位标回调函数

### 可变类型解决方案

* void* 这种方法不是类型安全的

* boost::any 可以将任何类型安全存储并且安全取回；可以在标准库程序中存放不同类型的数据，比如vector&lt;boost::any&gt;

### signal(SIGPIPE, SIG_IGN) 忽略SIGPIPE信号

SIGPIPE产生的原因：如果一个socket在接收到RST packet之后，程序仍然向这个socket写入数据，就会产生SIGPIPE信号

这种现象是很常见的，譬如说，当 client 连接到 server 之后，这时候 server 准备向 client 发送多条消息，但在发送消息之前，client 进程意外奔溃了，那么接下来 server 在发送多条消息的过程中，就会出现SIGPIPE信号。

* client 连接到 server 之后，client 进程意外奔溃，这时它会发送一个 FIN 给 server

* 此时 server 并不知道 client 已经奔溃了，所以它会发送第一条消息给 client。但 client 已经退出了，所以 client 的 TCP 协议栈会发送一个 RST 给 server

* server 在接收到 RST 之后，继续写入第二条消息。往一个已经收到 RST 的 socket 继续写入数据，将导致SIGPIPE信号，从而杀死 server

对 server 来说，为了不被SIGPIPE信号杀死，那就需要忽略SIGPIPE信号
