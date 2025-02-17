// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <muduo/base/CurrentThread.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Timestamp.h>

namespace muduo
{
namespace net
{

// 前向声明了两个类
class Channel;
class Poller;
///
/// Reactor, at most one per thread.
///
/// This is an interface class, so don't expose too much details.
class EventLoop : boost::noncopyable
{
 public:
  EventLoop();
  ~EventLoop();  // force out-line dtor, for scoped_ptr members.

  ///
  /// Loops forever.
  ///
  /// Must be called in the same thread as creation of the object.
  ///
  void loop();

  void quit();

  ///
  /// Time when poll returns, usually means data arrivial.
  ///
  Timestamp pollReturnTime() const { return pollReturnTime_; }

  // internal usage
  void updateChannel(Channel* channel);		// 在Poller中添加或者更新通道
  void removeChannel(Channel* channel);		// 从Poller中移除通道

  void assertInLoopThread()
  {
    if (!isInLoopThread())
    {
      abortNotInLoopThread();
    }
  }
  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
  // 确保线程只调用属于自己的eventloop
  static EventLoop* getEventLoopOfCurrentThread();

 private:
  void abortNotInLoopThread();

  void printActiveChannels() const; // DEBUG

  typedef std::vector<Channel*> ChannelList;
  
  bool looping_; /* atomic */ // 是否处于循环状态
  bool quit_; /* atomic */ // 是否退出循环
  // 对bool类型的操作为原子性的操作，因此多线程修改bool类型的变量不需要添加多余的保护
  bool eventHandling_; /* atomic */ // 当前是否处于事件处理的状态
  const pid_t threadId_;		// 当前对象所属线程ID
  Timestamp pollReturnTime_; // 调用poll函数所返回的时间戳
  boost::scoped_ptr<Poller> poller_; // poller和eventloop是组合关系
  // poller的生存期由eventloop来管理，使用智能指针不需要手动delete poller_
  ChannelList activeChannels_;		// Poller返回的活动通道
  Channel* currentActiveChannel_;	// 当前正在处理的活动通道
};

}
}
#endif  // MUDUO_NET_EVENTLOOP_H
