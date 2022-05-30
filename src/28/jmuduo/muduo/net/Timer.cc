// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/Timer.h>

using namespace muduo;
using namespace muduo::net;

AtomicInt64 Timer::s_numCreated_;

void Timer::restart(Timestamp now)
{
  if (repeat_)
  {
    // 重新计算下一个超时时刻
    // 当前时间+超时时间间隔
    expiration_ = addTime(now, interval_);
    // 采用值传递，TimeStamp是64bit，这里使用8字节的寄存器进行传值，不需要传递到堆栈当中 
  }
  else
  {
    expiration_ = Timestamp::invalid();
  }
}
