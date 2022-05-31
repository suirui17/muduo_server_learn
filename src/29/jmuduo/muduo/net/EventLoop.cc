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
#include <muduo/net/TimerQueue.h>

//#include <poll.h>
#include <boost/bind.hpp>

#include <sys/eventfd.h>

using namespace muduo;
using namespace muduo::net;

namespace
{
// 当前线程EventLoop对象指针
// 线程局部存储
__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 10000;

int createEventfd()
{
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0)
  {
    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return evtfd;
}

}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
  return t_loopInThisThread;
}

EventLoop::EventLoop()
  : looping_(false),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()), // 创建一个evtfd用于线程间通信
    wakeupChannel_(new Channel(this, wakeupFd_)), // 创建一个通道
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
  wakeupChannel_->setReadCallback(
      boost::bind(&EventLoop::handleRead, this));
  // we are always reading the wakeupfd
  wakeupChannel_->enableReading(); // 纳入poller管理
}

EventLoop::~EventLoop()
{
  ::close(wakeupFd_);
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
  quit_ = false;
  LOG_TRACE << "EventLoop " << this << " start looping";

  //::poll(NULL, 0, 5*1000);
  while (!quit_)
  {
    activeChannels_.clear();
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    //++iteration_;
    if (Logger::logLevel() <= Logger::TRACE)
    {
      printActiveChannels();
    }
    // TODO sort channel by priority
    eventHandling_ = true;
    for (ChannelList::iterator it = activeChannels_.begin();
        it != activeChannels_.end(); ++it)
    {
      currentActiveChannel_ = *it;
      currentActiveChannel_->handleEvent(pollReturnTime_);
    }
    currentActiveChannel_ = NULL;
    eventHandling_ = false;
    doPendingFunctors();
    // I/O线程也可以执行一部分计算任务
    // I/O不繁忙时，添加一些计算任务，I/O线程就不必处于阻塞状态
  }

  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_ = false;
}

// 该函数可以跨线程调用
void EventLoop::quit()
{
  quit_ = true;
  if (!isInLoopThread()) // 如果不是由当前线程调用的quit
  {
    wakeup(); // 唤醒当前IO线程
  }
}

// 在I/O线程中执行某个回调函数，该函数可以跨线程调用
void EventLoop::runInLoop(const Functor& cb)
{
  if (isInLoopThread())
  {
    // 如果是当前IO线程调用runInLoop，则同步调用cb
    cb();
  }
  else
  {
    // 如果是其它线程调用runInLoop，则异步地将cb添加到队列
    queueInLoop(cb);
  }
}

void EventLoop::queueInLoop(const Functor& cb)
{
  {
  MutexLockGuard lock(mutex_);
  pendingFunctors_.push_back(cb);
  }

  // 调用queueInLoop的线程不是当前IO线程，需要唤醒（其它线程向当前线程添加任务，需要唤醒I/O线程以便及时执行任务）
  // 或者调用queueInLoop的线程是当前IO线程，并且此时正在调用pending functor，需要唤醒
  // 只有当前IO线程的事件回调中调用queueInLoop才不需要唤醒，因为在执行完handleEvent之后，会执行doPendingFunctors
  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
  // callingPendingFunctors：在doPendingFunctors函数中调用queueInLoop
  // 此时需要唤醒当前线程，否则在线程在执行完doPendingFunctors之后，无法及时执行队列中的callback
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
  return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
  return timerQueue_->cancel(timerId);
}

void EventLoop::updateChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
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

void EventLoop::wakeup()
{
  uint64_t one = 1;
  //ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
  ssize_t n = ::write(wakeupFd_, &one, sizeof one);
  // 在wakeupfd中写入八个字节就可以唤醒等待进程
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

void EventLoop::handleRead()
{
  uint64_t one = 1;
  //ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
  ssize_t n = ::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

// Functor是回调任务
void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functors;
  callingPendingFunctors_ = true; // bool类型为原子操作，不需要进行临界区保护
  // 设置进程处于处理回调任务的状态中

  {
  MutexLockGuard lock(mutex_);
  functors.swap(pendingFunctors_);
  // pendingFunctors_变为空，functors中包含此前已经加入队列的回调任务
  }

  // 以下代码不受临界区保护
  // 减少临界区，不会阻塞其他线程的queueInLoop()
  // 避免死锁，因为Functors可能会再次调用queueInLoop
  for (size_t i = 0; i < functors.size(); ++i)
  {
    functors[i](); // 没有临界区保护
  }
  // doPendingFunctors()中调用Functors可能再次调用queueInLoop(cb)
  // 此时，queueInLoop()就必须wakeup()，否则新增加的cb就不能及时调用
  // 从loop中poll函数返回（wakeupFd）有可读事件，读取数据之后调用doPendingFunctors

  // muduo没有反复执行doPendingFunctors，直到PendingFunctors为空
  // 这是有意避免I/O线程陷入死循环，无法处理I/O事件

  callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const
{
  for (ChannelList::const_iterator it = activeChannels_.begin();
      it != activeChannels_.end(); ++it)
  {
    const Channel* ch = *it;
    LOG_TRACE << "{" << ch->reventsToString() << "} ";
  }
}
