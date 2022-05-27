#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include <muduo/base/Mutex.h>
#include <muduo/base/Types.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{

class LogFile : boost::noncopyable
{
 public:
  LogFile(const string& basename,
          size_t rollSize,
          bool threadSafe = true,
          int flushInterval = 3);
  ~LogFile();

  void append(const char* logline, int len);
  // 将一行添加到日志文件当中
  void flush();
  // 清空缓冲区

 private:
  void append_unlocked(const char* logline, int len);
  // 以不加锁的方式添加

  static string getLogFileName(const string& basename, time_t* now);
  // 获取日志文件的名称
  void rollFile();
  // 滚动日志

  const string basename_;		// 日志文件的basename
  const size_t rollSize_;		// 日志文件达到rollSize
  const int flushInterval_;		// 日志写入间隔时间

  int count_; // 计数器，初始值为0
  // 当其值达到kCheckTimeRoll时，需要检测是否需要换一个新的日志文件，或是否需要将日志写入到文件当中

  boost::scoped_ptr<MutexLock> mutex_;
  time_t startOfPeriod_;	// 开始记录日志时间（调整至零点的时间，即其距离时间纪元的秒数）
  time_t lastRoll_;			// 上一次滚动日志的时间
  time_t lastFlush_;		// 上一次日志写入文件的时间
  class File; // 嵌套类，这里只是一个前向声明
  boost::scoped_ptr<File> file_; // 智能指针

  const static int kCheckTimeRoll_ = 1024;
  const static int kRollPerSeconds_ = 60*60*24; // 多少秒进行一次日志滚动，60*60*24为一天的秒数
};

}
#endif  // MUDUO_BASE_LOGFILE_H
