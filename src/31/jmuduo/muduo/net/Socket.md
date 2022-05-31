### Socket封装

* Endian.h：封装了字节序转换函数（全局函数，位于muduo::net::sockets命名空间中）

* SocketsOps.h/SocketOps.cc：封装了socket相关系统调用（全局函数，位于muduo::net::sockets命名空间中）

* Socket.h/Socket.cc：用RAII方法封装socket file descriptor，即使用局部对象来管理资源的技术成为资源获取即初始化，局部对象是指存储在栈的对象，它的生命周期由操作系统来管理，无需人工介入

* InetAddress.h/InteAddress.cc：网际地址sockaddr_in封装

### 地址结构

```c++
// ipv4套接字地址结构
struct in_addr {
    in_addr_t s_addr;
};

struct sockaddr_in {
    uint8_t sin_len; // 地址结构的长度
    sa_family_t sin_family; // 协议族
    in_port_t sin_port; // 16-bitTCP和UDP端口号
    
    struct in_addr sin_addr; // 32-bit IPv4地址
    
    char sin_zero[8]; // 不使用，但要求置为0
}

// 通用套接字地址结构
struct sockaddr {
    uint8_t sa_len; // 通用套接字地址结构长度
    sa_family_t sa_family; // 协议族
    char sa_daya[14]; // 端口和地址等信息
};
```

### 相关函数

```c++
int getsockopt(int sock, int level, int optname, void* optval, socklen_t* optlen);
int setsockopt(int sock, int level, int optname, const void *optval, socklen_t optlen);
```

* sock：将要被设置或获取选项的套接字

* level：选项所在的协议层

* optname：需要访问的选项名

* optval：对于getsockopt，指向返回结果的缓冲；对于setsockopt，指向新选项值的缓冲

* optlen：对于getsockopt，作为入口参数时，表示选项值的最大长度，作为出口参数时，表示选项值的实际长度；对于setsockopt，新选项值的长度
