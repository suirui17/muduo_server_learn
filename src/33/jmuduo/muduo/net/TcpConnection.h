// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include <muduo/base/Mutex.h>
#include <muduo/base/StringPiece.h>
#include <muduo/base/Types.h>
#include <muduo/net/Callbacks.h>
//#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>

//#include <boost/any.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace muduo
{
namespace net
{

class Channel;
class EventLoop;
class Socket;

///
/// TCP connection, for both client and server usage.
///
/// This is an interface class, so don't expose too much details.
class TcpConnection : boost::noncopyable,
                      public boost::enable_shared_from_this<TcpConnection>
{
 public:
  /// Constructs a TcpConnection with a connected sockfd
  ///
  /// User should not create this object.
  TcpConnection(EventLoop* loop,
                const string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
  ~TcpConnection();

  EventLoop* getLoop() const { return loop_; }
  const string& name() const { return name_; }
  const InetAddress& localAddress() { return localAddr_; }
  const InetAddress& peerAddress() { return peerAddr_; }
  bool connected() const { return state_ == kConnected; }

  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

  // called when TcpServer accepts a new connection
  void connectEstablished();   // should be called only once

 private:
  enum StateE { /*kDisconnected, */kConnecting, kConnected/*, kDisconnecting*/ };
  void handleRead(Timestamp receiveTime);
  void setState(StateE s) { state_ = s; }

  EventLoop* loop_;			// 所属EventLoop
  string name_;				// 连接名
  StateE state_;  // FIXME: use atomic variable
  // we don't expose those classes to client.

  boost::scoped_ptr<Socket> socket_;
  boost::scoped_ptr<Channel> channel_;

  // 本地地址和对等方地址
  InetAddress localAddr_;
  InetAddress peerAddr_;
  
  // 两个回调函数
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
};

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

}
}

#endif  // MUDUO_NET_TCPCONNECTION_H
