// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <muduo/base/Timestamp.h>

namespace muduo
{
namespace net
{

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
// 不拥有fd
class Channel : boost::noncopyable
{
 public:
  typedef boost::function<void()> EventCallback; // 事件回调处理
  typedef boost::function<void(Timestamp)> ReadEventCallback; // 读事件的回调处理，需要多一个时间戳

  Channel(EventLoop* loop, int fd); // 一个所属eventloop
  ~Channel();

  void handleEvent(Timestamp receiveTime);
  void setReadCallback(const ReadEventCallback& cb)
  { readCallback_ = cb; }
  void setWriteCallback(const EventCallback& cb)
  { writeCallback_ = cb; }
  void setCloseCallback(const EventCallback& cb)
  { closeCallback_ = cb; }
  void setErrorCallback(const EventCallback& cb)
  { errorCallback_ = cb; }

  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in handleEvent.
  void tie(const boost::shared_ptr<void>&);

  int fd() const { return fd_; } // channel所对应的文件描述符
  int events() const { return events_; } // 注册的事件保存在events_里面
  void set_revents(int revt) { revents_ = revt; } // used by pollers
  // int revents() const { return revents_; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  void enableReading() { events_ |= kReadEvent; update(); } 
  // 关注可读事件，把通道注册到eventloop，从而注册到eventloop所持有的poller对象中
  // void disableReading() { events_ &= ~kReadEvent; update(); }
  void enableWriting() { events_ |= kWriteEvent; update(); }
  void disableWriting() { events_ &= ~kWriteEvent; update(); }
  void disableAll() { events_ = kNoneEvent; update(); }
  bool isWriting() const { return events_ & kWriteEvent; }

  // for Poller
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  // for debug
  // 将关注事件转成字符串以便调试
  string reventsToString() const;

  void doNotLogHup() { logHup_ = false; }

  EventLoop* ownerLoop() { return loop_; }
  void remove();

 private:
  void update();
  void handleEventWithGuard(Timestamp receiveTime);

  static const int kNoneEvent; // 没有事件
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop* loop_;			// 所属EventLoop
  const int  fd_;			// 文件描述符，但不负责关闭该文件描述符
  int        events_;		// 关注的事件
  int        revents_;		// poll/epoll返回的事件
  int        index_;		// used by Poller.表示channel在poll的事件数组中的序号
  bool       logHup_;		// for POLLHUP

  boost::weak_ptr<void> tie_;
  bool tied_;
  bool eventHandling_;		// 是否处于处理事件中
  // 事件回调函数
  ReadEventCallback readCallback_; 
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

}
}
#endif  // MUDUO_NET_CHANNEL_H
