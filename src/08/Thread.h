#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>

class Thread
{
public:
	Thread();
	virtual ~Thread(); // 多态基类应该使用纯虚析构函数，否则派生类的资源释放可能出现问题

	void Start();
	void Join(); 

	void SetAutoDelete(bool autoDelete);

private:
	static void* ThreadRoutine(void* arg); // 加static不会隐含this指针
	virtual void Run() = 0;
	pthread_t threadId_;
	bool autoDelete_;
};

#endif // _THREAD_H_
