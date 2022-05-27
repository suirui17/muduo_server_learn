// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/Thread.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Exception.h>
//#include <muduo/base/Logging.h>

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo
{
namespace CurrentThread
{
  // __thread修饰的变量是线程局部存储的。
  __thread int t_cachedTid = 0;		// 线程真实pid（tid）的缓存，
									// 是为了减少::syscall(SYS_gettid)系统调用的次数
									// 提高获取tid的效率
  __thread char t_tidString[32];	// 这是tid的字符串表示形式
  __thread const char* t_threadName = "unknown"; // 线程的名称
  const bool sameType = boost::is_same<int, pid_t>::value; // 检查pid_t是否为int类型
  BOOST_STATIC_ASSERT(sameType); // 编译时断言
}

namespace detail
{

pid_t gettid()
{
  return static_cast<pid_t>(::syscall(SYS_gettid));
  // 使用系统调用获取线程的真实id
}

/* fork之前可能有多个线程
 * fork可能是在主线程中调用，可能是在子线程中调用
 * fork如果是在子线程中调用，新进程只有一个执行序列，只有一个线程（调用fork的线程被继承下来），我们希望这个线程作为新进程的主线程
 * 如果是主线程调用，在新进程中其仍为主线程
 * 对于编写多线程来说，最好不要再调用fork
 * 即不要编写多线程多进程程序，要么使用多线程，要么使用多进程
 * fork只会复制当前进程的状态，而不会复制子进程的状态
 */
void afterFork()
{
  muduo::CurrentThread::t_cachedTid = 0;
  muduo::CurrentThread::t_threadName = "main";
  CurrentThread::tid();
  // no need to call pthread_atfork(NULL, NULL, &afterFork);
}


class ThreadNameInitializer
{
 public:
  ThreadNameInitializer()
  {
    muduo::CurrentThread::t_threadName = "main";
    // 主线程的名称为main
    CurrentThread::tid();
    // 缓存当前线程程（主进程）的id
    pthread_atfork(NULL, NULL, &afterFork);
    // 子进程会调用afterfork
  }
};

// 相当于全局变量，构造先于main函数，最开始就构造了
ThreadNameInitializer init;
}
}

using namespace muduo;

void CurrentThread::cacheTid()
{
  if (t_cachedTid == 0) // 当前线程的tid没有缓存
  {
    t_cachedTid = detail::gettid();
    int n = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    assert(n == 6); (void) n; 
    // (void) n; 防止n未使用变量报错
  }
}

bool CurrentThread::isMainThread()
{
  return tid() == ::getpid();
  // 查看tid是否是当前进程id，如果是，则为主线程
}

AtomicInt32 Thread::numCreated_;

Thread::Thread(const ThreadFunc& func, const string& n)
  : started_(false),
    pthreadId_(0),
    tid_(0),
    func_(func),
    name_(n)
{
  numCreated_.increment();
}

Thread::~Thread()
{
  // no join
}

void Thread::start()
{
  assert(!started_);
  started_ = true;
  errno = pthread_create(&pthreadId_, NULL, &startThread, this);
  if (errno != 0)
  {
    //LOG_SYSFATAL << "Failed in pthread_create";
  }
}

int Thread::join()
{
  assert(started_);
  return pthread_join(pthreadId_, NULL);
}

void* Thread::startThread(void* obj)
{
  Thread* thread = static_cast<Thread*>(obj);
  thread->runInThread();
  return NULL;
}

void Thread::runInThread()
{
  tid_ = CurrentThread::tid();
  muduo::CurrentThread::t_threadName = name_.c_str();
  try
  {
    func_(); // 调用回调函数 
    muduo::CurrentThread::t_threadName = "finished";
  }
  catch (const Exception& ex)
  {
    muduo::CurrentThread::t_threadName = "crashed";
    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
    abort();
  }
  catch (const std::exception& ex)
  {
    muduo::CurrentThread::t_threadName = "crashed";
    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    abort();
  }
  catch (...)
  {
    muduo::CurrentThread::t_threadName = "crashed";
    fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
    throw; // rethrow
  }
}

