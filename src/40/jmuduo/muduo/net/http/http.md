### http request

request line + header + body

header分为普通报头和实体报头

header和body之间有一空行（CRLF）

常用的请求头

* Accept：浏览器可接受的媒体类型

* Accept-Language：浏览器所希望的语言种类

* Accept-Encoding：浏览器所能够解码的编码方法，如gzip、deflate等

* User-Agent：告诉HTTP服务器，客户端使用的操作系统和浏览器的名称和版本

* Connection：表示是否需要持久连接，Keep-Alive表示长连接，close表示短连接

请求方法有：Get、Post、Head、Put、Delete

### http response

status line + header + body

header为普通报头、响应报头和实体报头

header和body之间有一空行（CRLF）

### muduo_http库设计到的类

* HttpRequest：对http请求类封装

* HttpResponse：http响应类封装

* HttpContext：http协议解析类

* HttpServer：http服务器类封装

### http协议

#### 简介

* Hyper Text Transfer Protocol 超文本传输协议，用于从万维网服务器传输文本到本地浏览器的传送协议

* http是基于TCP/IP通信协议来传送数据（html文件、图片文件、查询结果等）

* http是一个属于应用层的面向对象的协议，由于其简捷、快速的方式，适用于分布式超媒体信息系统

* http协议工作于客户端-服务端架构之上，浏览器作为http客户端通过URL向http服务端即web服务器发送请求，web服务器根据接收到的请求，向客户端发送响应信息

#### 主要特点

* 简单快速：客户端请求服务时，只需要传送请求方法和路径，请求方法有Get、Post、Head、Put、Delete，每种方法规定了客户与服务器联系的类型；由于http协议简单，使得http服务器的程序规模小，因而通信速度很快

* 灵活：http允许传输任意类型的数据对象，正在传输的类型由content-type加以标记

* 无连接：每次连接只处理一个请求，服务器处理完客户请求，并收到客户应答之后，即断开连接，**采用这种方式可以节省传输时间**

    * 客户端和服务器之间交换数据的间歇性较大，并且网页浏览的数据关联性较低，大部分通道实际上会很空闲，无端占用资源

    * 随着时间的推移，网页变得越来越复杂，里面可能嵌入了很多图片，这时候每次访问图片都需要建立一次 TCP 连接就显得很低效，Keep-Alive 被提出用来解决这效率低的问题；Keep-Alive 功能使客户端到服务器端的连接持续有效，当出现对服务器的后继请求时，Keep-Alive 功能避免了建立或者重新建立连接

* 无状态：http协议是无状态协议，对于事务处理没有记忆功能，如果后续处理前面的信息，则必须重传，这可能导致每次连接传送的数据量增大；在服务不需要先前信息时，可以很快应答

#### URL

* URI：Uniform Resource Identifiers，统一资源标识符，用于传输数据和建立连接

* URL：UniformResourceLocator，统一资源定位符，是一种特殊的URI，包含了用于查找某个资源的足够信息，是互联网上用于标识某一处资源的地址

#### http请求消息Request

```http
POST / HTTP1.1
Host:www.wrox.com
User-Agent:Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 2.0.50727; .NET CLR 3.0.04506.648; .NET CLR 3.5.21022)
Content-Type:application/x-www-form-urlencoded
Content-Length:40
Connection: Keep-Alive

name=Professional%20Ajax&publisher=Wiley
```

* 请求行line1：以方法名开头，空格分割，后跟请求的URI和协议的版本

* 请求头部line2-6：

    * host：指出请求目的地
    
    * user-agent：服务端和客户端都能访问，是浏览器类型检测逻辑的重要基础，该信息由浏览器来定义，并且在每个请求中自动发送

* 空行line7

* 请求数据line8

#### http响应消息Response

```http
HTTP/1.1 200 OK
Date: Fri, 22 May 2009 06:07:21 GMT
Content-Type: text/html; charset=UTF-8

<html>
      <head></head>
      <body>
            <!--body goes here-->
      </body>
</html>
```

* 状态行line1：http版本 状态码 状态消息

* 消息报头line2-3：用来说明客户端要使用的一些附加信息

    * date：生成响应的日期和时间

    * content-type：指定了MIME类型的html，编码类型是utf-8

* 空行line4

* 响应正文：服务器返回给客户端的文本信息

#### 状态响应码

* 1xx：提示信息，表示请求已被成功接收，继续处理

* 2xx：成功，表示请求已被成功接收、理解、接受

* 3xx：重定向，要完成请求必须进行进一步的处理

* 4xx：客户端错误，请求有语法错误或请求无法实现

* 5xx：服务器端错误，服务器执行一个有效请求失败

#### http请求方法

HTTP1.0定义了三种请求方法： GET、POST、HEAD

HTTP1.1新增了五种请求方法：OPTIONS、PUT、DELETE、TRACE、CONNECT

* GET：请求指定的页面信息，并返回实体主体

* HEAD：类似于get请求，只不过返回的响应中没有具体的内容，用于获取报头

* POST：向指定资源提交数据进行处理请求（例如提交表单或者上传文件）；数据被包含在请求体中；POST请求可能会导致新的资源的建立和/或已有资源的修改

* PUT：从客户端向服务器传送的数据取代指定的文档的内容

* DELETE：请求服务器删除指定的页面

* CONNECT：HTTP/1.1协议中预留给能够将连接改为管道方式的代理服务器

* OPTIONS：允许客户端查看服务器的性能

* TRACE：回显服务器收到的请求，主要用于测试或诊断
