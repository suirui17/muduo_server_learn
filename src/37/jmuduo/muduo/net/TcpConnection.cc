// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/TcpConnection.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Socket.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

// 在TcpServer中设置默认的回调函数
void muduo::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
  LOG_TRACE << conn->localAddress().toIpPort() << " -> "
            << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
}

void muduo::net::defaultMessageCallback(const TcpConnectionPtr&,
                                        Buffer* buf,
                                        Timestamp)
{
  buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
                             const string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
  : loop_(CHECK_NOTNULL(loop)),
    name_(nameArg),
    state_(kConnecting),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr)/*,
    highWaterMark_(64*1024*1024)*/
{
  // 通道可读事件到来的时候，回调TcpConnection::handleRead，_1是事件发生时间
  channel_->setReadCallback(
      boost::bind(&TcpConnection::handleRead, this, _1));
  // 连接关闭，回调TcpConnection::handleClose
  channel_->setCloseCallback(
      boost::bind(&TcpConnection::handleClose, this));
  // 发生错误，回调TcpConnection::handleError
  channel_->setErrorCallback(
      boost::bind(&TcpConnection::handleError, this));
  LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this
            << " fd=" << sockfd;
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
  LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd();
}

// 线程安全，可以跨线程调用
// 转到其所属的线程发送
void TcpConnection::send(const void* data, size_t len)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(data, len);
    }
    else
    {
      string message(static_cast<const char*>(data), len);
      loop_->runInLoop(
          boost::bind(&TcpConnection::sendInLoop,
                      this,
                      message));
    }
  }
}

// 线程安全，可以跨线程调用
void TcpConnection::send(const StringPiece& message)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(message);
    }
    else
    {
      loop_->runInLoop(
          boost::bind(&TcpConnection::sendInLoop,
                      this,
                      message.as_string()));
                    //std::forward<string>(message)));
    }
  }
}

// 线程安全，可以跨线程调用
void TcpConnection::send(Buffer* buf)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll(); // 将缓冲区中已发送数据移除（修改读指针）
    }
    else
    {
      loop_->runInLoop(
          boost::bind(&TcpConnection::sendInLoop,
                      this,
                      buf->retrieveAllAsString()));
                    //std::forward<string>(message)));
    }
  }
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
  loop_->assertInLoopThread();
  sockets::write(channel_->fd(), data, len);

  // Buffer类本身不是线程安全的
  // 每个Buffer是连接私有的
  /*
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool error = false;
  if (state_ == kDisconnected)
  {
    LOG_WARN << "disconnected, give up writing";
    return;
  }
  // if no thing in output queue, try writing directly
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
  {
    nwrote = sockets::write(channel_->fd(), data, len);
    if (nwrote >= 0)
    {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_)
      {
        loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
      }
    }
    else // nwrote < 0
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK)
      {
        LOG_SYSERR << "TcpConnection::sendInLoop";
        if (errno == EPIPE) // FIXME: any others?
        {
          error = true;
        }
      }
    }
  }

  assert(remaining <= len);
  if (!error && remaining > 0)
  {
    LOG_TRACE << "I am going to write more data";
    size_t oldLen = outputBuffer_.readableBytes();
    if (oldLen + remaining >= highWaterMark_
        && oldLen < highWaterMark_
        && highWaterMarkCallback_)
    {
      loop_->queueInLoop(boost::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
    }
    outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
    if (!channel_->isWriting())
    {
      channel_->enableWriting();
    }
  }
  */
}

// 应用层需要发送的数据都发送完毕才能关闭连接
// 不可以跨线程调用
void TcpConnection::shutdown()
{
  // FIXME: use compare and swap
  if (state_ == kConnected)
  {
    setState(kDisconnecting); // 正在断开连接
    // FIXME: shared_from_this()?
    loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop()
{
  loop_->assertInLoopThread();
  if (!channel_->isWriting()) // channel处于writting状态
  // 还在关注pollout事件，即Buffer还有数据没有发送完
  {
    // we are not writing
    // 如果channel已经不关注pollout事件，也就是应用层发送缓冲区数据已经全部写入内核
    // 则关闭tcp连接的写端
    // 服务器端主动断开与客户端的连接，客户端read为0，服务器端收到POLLHUP和POLLIN两个事件
    // 如果是客户端主动关闭连接的话，服务器端只会收到一个POLLIN
    socket_->shutdownWrite();
  }
  // 如果应用层发送缓冲区仍然有数据，则连接状态更改为kDisconnecting，但是并没有关闭连接
}

void TcpConnection::setTcpNoDelay(bool on)
{
  socket_->setTcpNoDelay(on);
}

void TcpConnection::connectEstablished()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  LOG_TRACE << "[3] usecount=" << shared_from_this().use_count();
  channel_->tie(shared_from_this());
  channel_->enableReading();	// TcpConnection所对应的通道加入到Poller关注

  connectionCallback_(shared_from_this());
  LOG_TRACE << "[4] usecount=" << shared_from_this().use_count();
}

void TcpConnection::connectDestroyed()
{
  loop_->assertInLoopThread();
  if (state_ == kConnected)
  {
    setState(kDisconnected);
    channel_->disableAll();

    connectionCallback_(shared_from_this());
  }
  channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
  loop_->assertInLoopThread();
  int savedErrno = 0;
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  if (n > 0)
  {
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
  }
  else if (n == 0)
  {
    handleClose();
  }
  else
  {
    errno = savedErrno;
    LOG_SYSERR << "TcpConnection::handleRead";
    handleError();
  }

  /*
  loop_->assertInLoopThread();
  int savedErrno = 0;
  char buf[65536];
  ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
  if (n > 0)
  {
    messageCallback_(shared_from_this(), buf, n);
  }
  else if (n == 0)
  {
    handleClose();
  }
  else
  {
    errno = savedErrno;
    LOG_SYSERR << "TcpConnection::handleRead";
    handleError();
  }
  */
}

void TcpConnection::handleClose()
{
  loop_->assertInLoopThread();
  LOG_TRACE << "fd = " << channel_->fd() << " state = " << state_;
  assert(state_ == kConnected || state_ == kDisconnecting);
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  setState(kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr guardThis(shared_from_this());
  connectionCallback_(guardThis);		// 这一行，可以不调用
  LOG_TRACE << "[7] usecount=" << guardThis.use_count();
  // must be the last line
  closeCallback_(guardThis);	// 调用TcpServer::removeConnection
  LOG_TRACE << "[11] usecount=" << guardThis.use_count();
}

void TcpConnection::handleError()
{
  int err = sockets::getSocketError(channel_->fd());
  LOG_ERROR << "TcpConnection::handleError [" << name_
            << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
