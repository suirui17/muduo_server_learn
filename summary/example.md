### muduo_http

#### HttpRequest：http请求类封装

```c++
class HttpRequest : public muduo::copyable
{
public:
    void addHeader(const char* start, const char* colon, const char* end);
    /* start和end给定了一个字符串，colon表示:的位置
     * 将给定请求头部fist:second映射为<first, second>
     */
    string getHeader(const string& field) const;
    /* 从headers_获得field(first)对应的second
     */

private:
    Method method_;		// 请求方法
    Version version_;		// 协议版本1.0/1.1
    string path_;			// 请求路径
    // 以上为请求行的信息

    Timestamp receiveTime_;	// 请求时间
    std::map<string, string> headers_;	// header列表
};
```

#### HttpResponse：http响应类封装

```c++
class HttpResponse : public muduo::copyable
{
public:
    void setContentType(const string& contentType);
    /* 设置文档媒体类型
     * 如addHeader("Content-Type", contentType);
     */

    void appendToBuffer(Buffer* output) const;	
    /* 将HttpResponse添加到缓冲区，不是应用层的发送缓冲区
     *      添加状态行
     *      添加实体长度（长连接）短连接不需要添加实体长度
     *      添加消息报头headers
     *      添加空行
     *      添加相应正文
     */


private:
    std::map<string, string> headers_;	// header列表
    HttpStatusCode statusCode_;			// 状态响应码
    // FIXME: add http version
    string statusMessage_;				// 状态响应码对应的文本信息
    bool closeConnection_;				// 是否关闭连接
    string body_;							// 实体
};
```

#### HttpContext：http协议解析类封装

```c++
// 协议解析状态机封装
class HttpContext : public muduo::copyable
{
public:
    // 状态转移函数
private:
    HttpRequestParseState state_;		// 请求解析状态
    HttpRequest request_;				// http请求
}
```

#### HttpServer：http服务器类封装

```c++
class HttpServer : boost::noncopyable
{
public:
    HttpServer(EventLoop* loop,
             const InetAddress& listenAddr,
             const string& name);
    /* 创建TcpServer
     * 初始化http回调函数
     * 设置TcpServer的连接建立回调函数 HttpServer::onConnection
     * 设置TcpServer销消息到来回调函数 HttpServer::onMessage
     */
    void start();
    /* server_.start();
     */
private:
    void onConnection(const TcpConnectionPtr& conn);
    /* conn->setContext(HttpContext());
     */
    void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp receiveTime);
    /* 将请求解析到conn的HttpContext的HttpRequest中
     * 如果解析成功，则调用HttpServer::onRequest
     * 请求处理完毕之后，重置HttpContex，适合于长连接
     */
    void onRequest(const TcpConnectionPtr&, const HttpRequest&);
    /* 调用用户回调函数httpCallback_，对http请求进行处理
     * 将得到的HttpResponse写入缓冲区
     * 通过TcpConnection将缓冲区中的数据发送（此时，才可能将数据添加到应用层的发送缓冲区
     */

    TcpServer server_;
    HttpCallback httpCallback_;	// 在处理http请求（即调用onRequest）的过程中回调此函数，对请求进行具体的处理
}
```

#### HttpServer处理流程

* 连接到来

    * poll/epoll函数返回
    
    * Channel::handleEvent

    * Channel::handleRead

    * Acceptor::handleRead

    * Acceptor::newConnectionCallback

    * TcpServer::newConnection

    * 选取io线程，创建TcpConnection，设置connectionCallback_为HttpServer::OnConnection，io线程运行TcpConnection::connectEstablished

    * TcpConnection::connectionCallback_ 即HttpServer::OnConnection，设置conn->setContext(HttpContext())

* 消息到来

    * poll/epoll函数返回
    
    * Channel::handleEvent

    * Channel::handleRead

    * TcpConnection::handleRead

    * TcpConnection::messageCallback_

    * HttpServer::Http::OnMessage，解析HttpRequest，保存至TcpConnection绑定的HttpContext的HttpRequest中

    * HttpServer::Http::OnRequest

    * HttpServer::httpCallback_，处理http请求，将HttpResponse保存至缓冲区并通过TcpConnection发送