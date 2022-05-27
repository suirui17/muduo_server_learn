#include <muduo/base/LogFile.h>
#include <muduo/base/Logging.h> // strerror_tl
#include <muduo/base/ProcessInfo.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace muduo;

// not thread safe
// 不是线程安全的，写入操作是通过不加锁的方式实现的
class LogFile::File : boost::noncopyable
{
 public:
  explicit File(const string& filename)
    : fp_(::fopen(filename.data(), "ae")), // 打开文件，并将文件描述符保存至fp_
      writtenBytes_(0)
  {
    assert(fp_);
    ::setbuffer(fp_, buffer_, sizeof buffer_);
    // void setbuffer(FILE * stream,char * buf,size_t size);
    // 打开文件流之后，读取内容之前，调用setbuffer()可用来设置文件流的缓冲区
    // 参数stream为指定的文件流，参数buf指向自定的缓冲区起始地址，参数size为缓冲区大小
    // posix_fadvise POSIX_FADV_DONTNEED ?
  }

  ~File()
  {
    ::fclose(fp_);
  }

  void append(const char* logline, const size_t len)
  {
    size_t n = write(logline, len);
    size_t remain = len - n;
	// remain>0表示没有写完目标行
    while (remain > 0)
    {
      size_t x = write(logline + n, remain);
      if (x == 0)
      {
        int err = ferror(fp_);
        if (err)
        {
          fprintf(stderr, "LogFile::File::append() failed %s\n", strerror_tl(err));
        }
        break;
      }
      n += x;
      remain = len - n; // remain -= x
    }

    writtenBytes_ += len;
  }

  void flush()
  {
    ::fflush(fp_); // 将缓冲内容写入文件
  }

  size_t writtenBytes() const { return writtenBytes_; }

 private:

  // 以不加锁的方式实现
  size_t write(const char* logline, size_t len)
  {
#undef fwrite_unlocked
    return ::fwrite_unlocked(logline, 1, len, fp_);
  }

  FILE* fp_; // 文件指针
  char buffer_[64*1024]; // 文件指针缓冲区 64k
  // 缓冲区写满会自动flush，不需要手动flush
  size_t writtenBytes_; // 已经写入文件的字节数
};

LogFile::LogFile(const string& basename,
                 size_t rollSize,
                 bool threadSafe,
                 int flushInterval)
  : basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    count_(0),
    mutex_(threadSafe ? new MutexLock : NULL), // 如果是线程安全的，需要初始化一个锁
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0)
{
  assert(basename.find('/') == string::npos);
  // 断言：判断basename是否包含/
  rollFile();
  // 滚动日志产生一个文件
}

LogFile::~LogFile()
{
}

void LogFile::append(const char* logline, int len)
{
  if (mutex_)
  {
    MutexLockGuard lock(*mutex_);
    append_unlocked(logline, len);
  }
  else
  {
    append_unlocked(logline, len);
  }
}

void LogFile::flush()
{
  if (mutex_)
  {
    MutexLockGuard lock(*mutex_);
    file_->flush();
  }
  else
  {
    file_->flush();
  }
}

void LogFile::append_unlocked(const char* logline, int len)
{
  file_->append(logline, len);

  // 写入文件，需要根据文件大小判断是否需要滚动文件
  if (file_->writtenBytes() > rollSize_)
  {
    rollFile();
  }
  else
  {
    if (count_ > kCheckTimeRoll_)
    {
      count_ = 0;
      time_t now = ::time(NULL);
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
      if (thisPeriod_ != startOfPeriod_)
      {
        rollFile();
      }
      else if (now - lastFlush_ > flushInterval_)
      // 是否超过了刷新间隔时间
      {
        lastFlush_ = now;
        file_->flush();
      }
    }
    else 
    {
      ++count_;
    }
  }
}

void LogFile::rollFile()
{
  time_t now = 0;
  string filename = getLogFileName(basename_, &now);
  // 获取文件名称并返回时间
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;
  // 对齐到kRollPerSeconds_的整数倍，即将时间对齐到当天零点

  if (now > lastRoll_) // 到了新的一天
  {
    lastRoll_ = now; // 更新最后滚动时间
    lastFlush_ = now; // 更新最后刷新时间
    startOfPeriod_ = start;
    file_.reset(new File(filename)); // 产生一个新的日志文件
  }
}

string LogFile::getLogFileName(const string& basename, time_t* now)
{
  string filename;
  filename.reserve(basename.size() + 64);
  // 为filename保存basename.size() + 64的空间
  filename = basename;

  char timebuf[32];
  char pidbuf[32];
  struct tm tm;
  *now = time(NULL); // 获取当前时间
  gmtime_r(now, &tm); // FIXME: localtime_r ? UTC时间
  // gmtime不是线程完全的，因为其返回一个指针，指向一个缓冲区
  // 如果两个线程同时调用gmtime，返回的数据可能会被另外一个线程更改
  // gmtime_r是线程安全的，返回的时候，除了返回值，还会将结果保存到tm当中
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
  // 将时间按照给定格式格式化到缓冲区当中
  filename += timebuf; // 在日志文件名中添加时间
  filename += ProcessInfo::hostname(); // 在日志文件名中添加主机名
  snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
  filename += pidbuf; // 在日志文件名中添加线程id
  filename += ".log";

  return filename;
}

