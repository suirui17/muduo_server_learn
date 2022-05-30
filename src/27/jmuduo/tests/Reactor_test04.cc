#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

#include <stdio.h>
#include <sys/timerfd.h>

using namespace muduo;
using namespace muduo::net;

Eventloop * g_loop; // 构造一个全局的eventloop指针
int timefd;

void timeout(imestamp receiveTime)
{
    printf("Timeout!\n");
    uint64_t howmany;
    ::read(timerfd, &howmany, sizeof howmany);
    // poll是电平触发，不读出可读数据的话，会一直触发可读事件
}

int main(void)
{
    Eventloop loop; // 构造一个eventloop对象
    g_loop = &loop; // 全局指针指向该对象

    timefd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    Channel channel(&loop, timerfd);
    // 创建一个channel，所属一个eventloop，对一个文件描述符（I/O事件）的注册与响应的封装
    channel.setReadCallback(boost::bind(timeout, _1));
    // setReadCallback函数接收的函数类型：typedef boost::function<void(Timestamp)> ReadEventCallback
    channel.enableReading(); // 关注可读事件

    struct itimerspec howlong;
    bzero(&howlong, sizeof howlong);
    howlong.it_value.tc_sec = 1;
    // 一次性定时器
    // muduo库在实现间隔性计时器时没有使用it_interval字段，而是每次重新注册一次性计时器
    ::timerfd_settime(timerfd, 0, &howlong, NULL);

    loop.loop(); // 定时器超时，loop会收到可读事件，会回调之前设置的readcallback函数
    // 定时器超时时间处理完成之后，loop会继续监测其他事件

    ::close(timerfd);
}