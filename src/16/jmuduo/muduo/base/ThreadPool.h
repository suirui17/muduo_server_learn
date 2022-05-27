// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Types.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <deque>

namespace muduo
{

class ThreadPool : boost::noncopyable
{
 public:
  typedef boost::function<void ()> Task;

  explicit ThreadPool(const string& name = string());
  ~ThreadPool();

  void start(int numThreads); // 启动线程固定
  void stop();

  void run(const Task& f); // 添加任务

 private:
  void runInThread();
  Task take(); // 获取任务

  MutexLock mutex_;
  Condition cond_;
  string name_;
  boost::ptr_vector<muduo::Thread> threads_; // 线程队列，其中存放的是线程指针
  std::deque<Task> queue_; // 任务队列
  bool running_;
};

}

#endif
