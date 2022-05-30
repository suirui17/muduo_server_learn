// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include <boost/noncopyable.hpp>

#include <muduo/base/Mutex.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/Channel.h>

namespace muduo
{
namespace net
{

class EventLoop;
class Timer;
class TimerId;

///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///
class TimerQueue : boost::noncopyable
{
 public:
  TimerQueue(EventLoop* loop);
  // 一个Timer属于一个eventloop，eventloop对象在某个IO线程中创建
  ~TimerQueue();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if @c interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
  // 一定是线程安全的，可以跨线程调用。通常情况下被其它线程调用。
  // 即可以不被所属的eventloop对象所在线程调用，可以被其他线程调用
  TimerId addTimer(const TimerCallback& cb,
                   Timestamp when,
                   double interval);

  // 也可以跨线程调用
  void cancel(TimerId timerId);

 private:

  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  // unique_ptr是C++ 11标准的一个独享所有权的智能指针
  // 无法得到指向同一对象的两个unique_ptr指针
  // 但可以进行移动构造与移动赋值操作，即所有权可以移动到另一个对象（而非拷贝构造）
  typedef std::pair<Timestamp, Timer*> Entry;
  // 组成一个键值对
  typedef std::set<Entry> TimerList;
  typedef std::pair<Timer*, int64_t> ActiveTimer;
  typedef std::set<ActiveTimer> ActiveTimerSet;
  // 以上两个set保存的是相同的东西，TimerList按照时间戳排序，ActiveTimerSet按照计时器地址排序

  // 以下成员函数只可能在其所属的I/O线程中调用，因而不必加锁。
  // 服务器性能杀手之一是锁竞争，所以要尽可能少用锁
  // 不能跨线程调用，就没有线程竞争
  void addTimerInLoop(Timer* timer);
  void cancelInLoop(TimerId timerId);

  // called when timerfd alarms
  void handleRead();

  // move out all expired timers
  // 返回超时的定时器列表
  std::vector<Entry> getExpired(Timestamp now);
  // 对超时的重复定时器进行重置
  void reset(const std::vector<Entry>& expired, Timestamp now);

  bool insert(Timer* timer);

  EventLoop* loop_;		// 所属EventLoop
  const int timerfd_; // ::timerfd_create创建的定时器文件描述符
  Channel timerfdChannel_; // 定时器通道
  // Timer list sorted by expiration
  TimerList timers_;	// timers_是按到期时间排序

  // for cancel()
  // timers_与activeTimers_保存的是相同的数据
  // timers_是按到期时间排序，activeTimers_是按对象地址排序
  ActiveTimerSet activeTimers_;
  bool callingExpiredTimers_; /* atomic */
  // 是否正在处理超时的计时器
  ActiveTimerSet cancelingTimers_;	// 保存的是被取消的定时器
};

}
}
#endif  // MUDUO_NET_TIMERQUEUE_H
