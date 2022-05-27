// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
// 基于对象的方法


#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include <muduo/base/Atomic.h>
#include <muduo/base/Types.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{

class Thread : boost::noncopyable
{
 public:
  typedef boost::function<void ()> ThreadFunc;

  explicit Thread(const ThreadFunc&, const string& name = string());
  // 默认名字为空字符串
  ~Thread();

  void start();
  int join(); // return pthread_join()

  bool started() const { return started_; }
  // pthread_t pthreadId() const { return pthreadId_; }
  pid_t tid() const { return tid_; }
  const string& name() const { return name_; }

  static int numCreated() { return numCreated_.get(); }

 private:
  static void* startThread(void* thread);
  void runInThread();

  bool       started_; // 线程是否已经启动
  pthread_t  pthreadId_;  // 进程独立id空间中的线程id
  pid_t      tid_; // 线程真实id
  ThreadFunc func_; // 基于对象的实现方式，回调函数
  string     name_;

  static AtomicInt32 numCreated_; // 静态成员，原子操作，每构建一个线程就+1
  // 构造函数将其初始化为0
};

}
#endif
