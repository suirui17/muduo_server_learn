// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/TcpServer.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Acceptor.h>
#include <muduo/net/EventLoop.h>
//#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <stdio.h>  // snprintf

using namespace muduo;
using namespace muduo::net;

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const string& nameArg)
  : loop_(CHECK_NOTNULL(loop)),
    hostport_(listenAddr.toIpPort()),
    name_(nameArg),
    acceptor_(new Acceptor(loop, listenAddr)),
    /*threadPool_(new EventLoopThreadPool(loop)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),*/
    started_(false),
    nextConnId_(1)
{
  // Acceptor::handleRead函数中会回调用TcpServer::newConnection
  // _1对应的是socket文件描述符，_2对应的是对等方的地址(InetAddress)
  acceptor_->setNewConnectionCallback(
      boost::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
  loop_->assertInLoopThread();
  LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

  for (ConnectionMap::iterator it(connections_.begin());
      it != connections_.end(); ++it)
  {
    TcpConnectionPtr conn = it->second;
    it->second.reset();		// 释放当前所控制的对象，引用计数减一
    conn->getLoop()->runInLoop(
      boost::bind(&TcpConnection::connectDestroyed, conn));
    conn.reset();			// 释放当前所控制的对象，引用计数减一
  }
}

// 该函数多次调用是无害的
// 该函数可以跨线程调用
void TcpServer::start()
{
  if (!started_)
  {
    started_ = true;
  }

  if (!acceptor_->listenning())
  {
	// get_pointer返回原生指针
    loop_->runInLoop(
        boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
  }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
  loop_->assertInLoopThread();
  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", hostport_.c_str(), nextConnId_);
  ++nextConnId_;
  string connName = name_ + buf;

  LOG_INFO << "TcpServer::newConnection [" << name_
           << "] - new connection [" << connName
           << "] from " << peerAddr.toIpPort();
  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary

  // 新建连接时，创建一个TcpConnection对象，使用shared_ptr对TcpConnection对象进行管理
  // TcpConnection引用计数为1
  // 将shared_ptr<TcpConnection>加入到TcpServer的TcpConnection列表中
  TcpConnectionPtr conn(new TcpConnection(loop_,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));

  LOG_TRACE << "[1] usecount=" << conn.use_count();
  connections_[connName] = conn;
  LOG_TRACE << "[2] usecount=" << conn.use_count();

  // 设置回调函数
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);

  conn->setCloseCallback(
      boost::bind(&TcpServer::removeConnection, this, _1));

  // 设置连接为建立状态
  // 该函数中，在cahnnel中维护weak_ptr(tie_)为TcpConnection的弱引用
  // 此时，TcpConnection的引用仍为1
  // 关注读事件，将channel加入到poller的关注列表中
  // 调用连接回调函数
  conn->connectEstablished();
  LOG_TRACE << "[5] usecount=" << conn.use_count();

}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
           << "] - connection " << conn->name();


  LOG_TRACE << "[8] usecount=" << conn.use_count();
  size_t n = connections_.erase(conn->name());
  // 将TcpConnection对象从列表中移除
  LOG_TRACE << "[9] usecount=" << conn.use_count();

  (void)n;
  assert(n == 1);
  
  // 放入eventloop的functors中
  loop_->queueInLoop(
      boost::bind(&TcpConnection::connectDestroyed, conn));
      // 该TcpConnection的引用计数会+1
  LOG_TRACE << "[10] usecount=" << conn.use_count();

}
