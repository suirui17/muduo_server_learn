// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_HTTP_HTTPCONTEXT_H
#define MUDUO_NET_HTTP_HTTPCONTEXT_H

#include <muduo/base/copyable.h>

#include <muduo/net/http/HttpRequest.h>

namespace muduo
{
namespace net
{

class HttpContext : public muduo::copyable
{
 public:
  enum HttpRequestParseState
  {
    kExpectRequestLine, // 解析请求行状态
    kExpectHeaders, // 解析头部状态
    kExpectBody, // 解析实体状态
    kGotAll, // 全部解析完毕
  };

  HttpContext()
    : state_(kExpectRequestLine)
  {
  }

  // default copy-ctor, dtor and assignment are fine

  bool expectRequestLine() const
  { return state_ == kExpectRequestLine; }

  bool expectHeaders() const
  { return state_ == kExpectHeaders; }

  bool expectBody() const
  { return state_ == kExpectBody; }

  bool gotAll() const
  { return state_ == kGotAll; }

  void receiveRequestLine()
  { state_ = kExpectHeaders; }

  void receiveHeaders()
  { state_ = kGotAll; }  // FIXME

  // 重置HttpContext状态
  void reset()
  {
    state_ = kExpectRequestLine; // 设置为初始状态
    HttpRequest dummy;
    request_.swap(dummy); // 当前请求置空
  }

  const HttpRequest& request() const
  { return request_; }

  HttpRequest& request()
  { return request_; }

 private:
  HttpRequestParseState state_;		// 请求解析状态
  HttpRequest request_;				// http请求
};

}
}

#endif  // MUDUO_NET_HTTP_HTTPCONTEXT_H
