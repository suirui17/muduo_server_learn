#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>
#include <boost/function.hpp>

class Thread
{
public:
	typedef boost::function<void ()> ThreadFunc; // run方法中调用
	// 面向对象编程中，是复写基类的run方法，run被定义为纯虚函数，子类必须复写基类的run方法
	// 基于对象编程中，给当前类传递了一个ThreadFunc方法，在run函数中调用该方法
	explicit Thread(const ThreadFunc& func);
	// 不允许隐式转换

	void Start();
	void Join();

	void SetAutoDelete(bool autoDelete);

private:
	static void* ThreadRoutine(void* arg);
	void Run();
	ThreadFunc func_;
	pthread_t threadId_;
	bool autoDelete_;
};

#endif // _THREAD_H_
