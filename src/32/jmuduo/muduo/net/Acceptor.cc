// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/Acceptor.h>

#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/stat.h>

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
  : loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie()),
    acceptChannel_(loop, acceptSocket_.fd()),
    listenning_(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) // 预先准备一个空闲的文件描述符
{
  assert(idleFd_ >= 0);
  acceptSocket_.setReuseAddr(true); // 设置地址重复利用
  acceptSocket_.bindAddress(listenAddr); // 绑定监听地址
  acceptChannel_.setReadCallback(
      boost::bind(&Acceptor::handleRead, this)); // 设置读回调函数
}

Acceptor::~Acceptor()
{
  acceptChannel_.disableAll(); // 所有事件都设置为不关注
  acceptChannel_.remove(); // 将acceptChannel从eventloop的channel列表中移除
  ::close(idleFd_);
}

void Acceptor::listen()
{
  loop_->assertInLoopThread();
  listenning_ = true;
  acceptSocket_.listen(); // 监听套接字
  acceptChannel_.enableReading(); // 关注channel的可读事件
}

void Acceptor::handleRead()
{
  loop_->assertInLoopThread();
  InetAddress peerAddr(0);
  //FIXME loop until no more
  int connfd = acceptSocket_.accept(&peerAddr); 
  if (connfd >= 0)
  {
    // string hostport = peerAddr.toIpPort();
    // LOG_TRACE << "Accepts of " << hostport;
    if (newConnectionCallback_)
    {
      newConnectionCallback_(connfd, peerAddr);
    }
    else
    {
      sockets::close(connfd);
    }
  }
  else
  {
    // Read the section named "The special problem of
    // accept()ing when you can't" in libev's doc.
    // By Marc Lehmann, author of livev.
    if (errno == EMFILE) // 文件描述符过多
    {
      ::close(idleFd_); // 关闭空闲描述符
      idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL); // 接收文件描述符
      ::close(idleFd_); // 断开与客户端连接
      idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC); // 重新打开空闲描述符
    }
  }
}

