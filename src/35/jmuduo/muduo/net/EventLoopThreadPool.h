// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MUDUO_NET_EVENTLOOPTHREADPOOL_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace muduo
{

namespace net
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : boost::noncopyable
{
 public:
  typedef boost::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThreadPool(EventLoop* baseLoop);
  ~EventLoopThreadPool();
  void setThreadNum(int numThreads) { numThreads_ = numThreads; }
  void start(const ThreadInitCallback& cb = ThreadInitCallback());
  EventLoop* getNextLoop();

 private:

  EventLoop* baseLoop_;	// 与Acceptor所属EventLoop相同
  bool started_;
  int numThreads_;		// 线程数
  int next_;			// 新连接到来，所选择的EventLoop对象下标
  // 即选定的EventLoop对象对应的线程来处理这个连接
  boost::ptr_vector<EventLoopThread> threads_;		// IO线程列表
  // 当ptr_vector对象销毁时，它所管理的EventLoopThread对象也会跟着销毁
  std::vector<EventLoop*> loops_;					// EventLoop列表
  // 一个I/O线程对应一个EventLoop对象，这些对象都是栈上的对象，因而不需要使用ptr_vector
};

}
}

#endif  // MUDUO_NET_EVENTLOOPTHREADPOOL_H
