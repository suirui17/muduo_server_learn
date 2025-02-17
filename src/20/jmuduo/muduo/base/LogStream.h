#ifndef MUDUO_BASE_LOGSTREAM_H
#define MUDUO_BASE_LOGSTREAM_H

#include <muduo/base/StringPiece.h>
#include <muduo/base/Types.h>
#include <assert.h>
#include <string.h> // memcpy
#ifndef MUDUO_STD_STRING
#include <string>
#endif
#include <boost/noncopyable.hpp>

namespace muduo
{

namespace detail
{

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000*1000;

template<int SIZE>
class FixedBuffer : boost::noncopyable // 缓冲区类
{
 public:
  FixedBuffer()
    : cur_(data_)
  {
    setCookie(cookieStart);
  }

  ~FixedBuffer()
  {
    setCookie(cookieEnd);
  }

// 在缓冲区中添加buf开始，长度为len的字符序列
  void append(const char* /*restrict*/ buf, size_t len)
  {
    // FIXME: append partially
    if (implicit_cast<size_t>(avail()) > len) // avail()返回值由int转化为size_t类型
    {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  const char* data() const { return data_; }
  // 字符数组首地址，即data的起始
  int length() const { return static_cast<int>(cur_ - data_); }
  // 当前缓冲区存放的字符数

  // write to data_ directly
  char* current() { return cur_; } 
  // 返回当前位置指针
  int avail() const { return static_cast<int>(end() - cur_); }
  // 当前还有多少位置可用
  void add(size_t len) { cur_ += len; }

  void reset() { cur_ = data_; }

  //把日志数据清零
  void bzero() { ::bzero(data_, sizeof data_); }

  // for used by GDB
  const char* debugString();
  void setCookie(void (*cookie)()) { cookie_ = cookie; }
  // 传递并设置函数指针
  // for used by unit test
  string asString() const { return string(data_, length()); }

 private:
  const char* end() const { return data_ + sizeof data_; }
  // 缓冲区结束位置
  // Must be outline function for cookies.
  static void cookieStart();
  static void cookieEnd();

  void (*cookie_)(); // 函数指针
  char data_[SIZE]; // 字符数组
  char* cur_; // 当前位置的指针
};

}

class LogStream : boost::noncopyable
{
  typedef LogStream self;
 public:
  typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;

  self& operator<<(bool v)
  {
    buffer_.append(v ? "1" : "0", 1);
    return *this;
  }

  self& operator<<(short);
  self& operator<<(unsigned short);
  self& operator<<(int);
  self& operator<<(unsigned int);
  self& operator<<(long);
  self& operator<<(unsigned long);
  self& operator<<(long long);
  self& operator<<(unsigned long long);

  self& operator<<(const void*);

  self& operator<<(float v)
  {
    *this << static_cast<double>(v);
    return *this;
  }
  self& operator<<(double);
  // self& operator<<(long double);

  self& operator<<(char v)
  {
    buffer_.append(&v, 1);
    return *this;
  }

  // self& operator<<(signed char);
  // self& operator<<(unsigned char);

  self& operator<<(const char* v)
  {
    buffer_.append(v, strlen(v));
    return *this;
  }

  self& operator<<(const string& v)
  {
    buffer_.append(v.c_str(), v.size());
    return *this;
  }

#ifndef MUDUO_STD_STRING
  self& operator<<(const std::string& v)
  {
    buffer_.append(v.c_str(), v.size());
    return *this;
  }
#endif

  self& operator<<(const StringPiece& v)
  {
    buffer_.append(v.data(), v.size());
    return *this;
  }

  void append(const char* data, int len) { buffer_.append(data, len); }
  const Buffer& buffer() const { return buffer_; }
  void resetBuffer() { buffer_.reset(); }

 private:
  void staticCheck();

  template<typename T>
  void formatInteger(T);

  Buffer buffer_;

  static const int kMaxNumericSize = 32;
};

class Fmt // : boost::noncopyable
{
 public: 
  template<typename T>
  Fmt(const char* fmt, T val); // 成员模板，将一个整数格式化

  const char* data() const { return buf_; }
  int length() const { return length_; }

 private:
  char buf_[32];
  int length_;
};

inline LogStream& operator<<(LogStream& s, const Fmt& fmt)
{
  s.append(fmt.data(), fmt.length());
  return s; 
}

}
#endif  // MUDUO_BASE_LOGSTREAM_H

