// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/EventLoop.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/Poller.h>

//#include <poll.h>

using namespace muduo;
using namespace muduo::net;

namespace
{
// 当前线程EventLoop对象指针
// 线程局部存储
__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 10000;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
  return t_loopInThisThread;
}

EventLoop::EventLoop()
  : looping_(false),
    quit_(false),
    eventHandling_(false),
    threadId_(CurrentThread::tid()),
	poller_(Poller::newDefaultPoller(this)),
	currentActiveChannel_(NULL)
{
  LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
  // 如果当前线程已经创建了EventLoop对象，终止(LOG_FATAL)
  if (t_loopInThisThread)
  {
    LOG_FATAL << "Another EventLoop " << t_loopInThisThread
              << " exists in this thread " << threadId_;
  }
  else
  {
    t_loopInThisThread = this;
  }
}

EventLoop::~EventLoop()
{
  t_loopInThisThread = NULL;
}

// 事件循环，该函数不能跨线程调用
// 只能在创建该对象的线程中调用
void EventLoop::loop()
{
  assert(!looping_);
  // 断言当前处于创建该对象的线程中
  assertInLoopThread();
  looping_ = true;
  LOG_TRACE << "EventLoop " << this << " start looping";

  //::poll(NULL, 0, 5*1000);
  while (!quit_)
  {
    activeChannels_.clear(); // 活动通道列表清空
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    //++iteration_;
    if (Logger::logLevel() <= Logger::TRACE)
    {
      printActiveChannels();
    }
    // TODO sort channel by priority
    eventHandling_ = true; // 处于事件处理状态
    for (ChannelList::iterator it = activeChannels_.begin();
        it != activeChannels_.end(); ++it)
    {
      // 处理当前事件
      currentActiveChannel_ = *it;
      currentActiveChannel_->handleEvent(pollReturnTime_);
    }
    currentActiveChannel_ = NULL;
    eventHandling_ = false;
    //doPendingFunctors();
  }

  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_ = false;
}

// 该函数可以跨线程调用，不需要每次都在IO线程中调用
// 如果不是下IO线程中调用需要wakeup
void EventLoop::quit()
{
  quit_ = true;
  // 将quit_置为true之后，上面的loop()函数中的while循环就会退出
  if (!isInLoopThread())
  // 如果不是在IO线程中调用quit，可能IO线程还处于handlevent的状态或者阻塞在poll位置
  {
    //wakeup(); // 唤醒阻塞线程
  }
}

void EventLoop::updateChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  // channel所属的eventloop对象为本对象才会进行处理
  assertInLoopThread(); // 判断当前线程是否在loop循环当中
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  if (eventHandling_)
  {
    assert(currentActiveChannel_ == channel ||
        std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
  }
  poller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();
}

// 对活动通道进行日志记录
void EventLoop::printActiveChannels() const
{
  for (ChannelList::const_iterator it = activeChannels_.begin();
      it != activeChannels_.end(); ++it)
  {
    const Channel* ch = *it;
    LOG_TRACE << "{" << ch->reventsToString() << "} ";
  }
}
