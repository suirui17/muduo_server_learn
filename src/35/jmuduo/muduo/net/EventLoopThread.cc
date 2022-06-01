// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/EventLoopThread.h>

#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;


EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
  : loop_(NULL),
    exiting_(false),
    thread_(boost::bind(&EventLoopThread::threadFunc, this)),
    mutex_(),
    cond_(mutex_),
    callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
  exiting_ = true;
  loop_->quit();		// 退出IO线程，让IO线程的loop循环退出，从而退出了IO线程
  thread_.join();
}

EventLoop* EventLoopThread::startLoop()
{
  assert(!thread_.started());
  thread_.start(); // 创建并启动线程，并执行绑定的线程函数EventLoopThread::threadFunc

  {
    MutexLockGuard lock(mutex_); // 使用条件变量，等待eventloop创建
    while (loop_ == NULL)
    {
      cond_.wait();
    }
  }

  return loop_; // 返回EventLoop对象
}

void EventLoopThread::threadFunc()
{
  EventLoop loop;

  if (callback_) // 如果回调函数不为空
  {
    callback_(&loop);
  }

  {
    MutexLockGuard lock(mutex_);
    // loop_指针指向了一个栈上的对象，threadFunc函数退出之后，这个指针就失效了
    // threadFunc函数退出，就意味着线程退出了，EventLoopThread对象也就没有存在的价值了。
    // 因而不会有什么大的问题
    loop_ = &loop; // 创建EventLoop对象并将loop_指针指向该对象
    cond_.notify(); // 通知正在等待的EventLoop对象创建的EventLoopThread::startLoop()函数
  }

  loop.loop(); //开始运行，poll监听关注事件
  //assert(exiting_);
}

