### muduo_inspect

通过http方式为服务器提供监控接口

* 接受了多少TCP连接

* 当前有多少活动连接

* 一共响应了多少次请求

* 每次请求的平均相应时间多少毫秒

* ...

Inspector：包含一个HttpServer对象

ProcessInspector：通过ProcessInfo返回进程信息

ProcessInfo：获取进程相关信息

### 竞态问题

Inspector一般在主线程中构造对象，并且构造一个监控线程

如果直接调用start，则调用start的线程不是loop所属的I/O线程，而是主线程，那么有可能构造函数还没有返回，I/O线程（http server所在的线程）就已经收到了http客户端的请求 ，就会回调Inspector::OnRequest函数，但是构造函数还没返回，Inspector对象还没完全构造好