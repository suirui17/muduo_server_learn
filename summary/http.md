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

状态响应码

* 1xx：提示信息，表示请求已被成功接收，继续处理

* 2xx：成功，表示请求已被成功接收、理解、接受

* 3xx：重定向，要完成请求必须进行进一步的处理

* 4xx：客户端错误，请求有语法错误或请求无法实现

* 5xx：服务器端错误，服务器执行一个有效请求失败

### muduo_http库设计到的类

* HttpRequest：对http请求类封装

* HttpResponse：http响应类封装

* HttpContext：http协议解析类

* HttpServer：http服务器类封装

