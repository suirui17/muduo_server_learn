#include "Thread.h"
#include <iostream>
using namespace std;


Thread::Thread() : autoDelete_(false)
{
	cout<<"Thread ..."<<endl;
}

Thread::~Thread()
{
	cout<<"~Thread ..."<<endl;
}

void Thread::Start()
{
	pthread_create(&threadId_, NULL, ThreadRoutine, this);
	// 线程ID
	// 线程属性（null表示默认属性)
	// 线程入口函数,不可以直接使用run函数，
	// 		run函数是普通的成员函数，隐含的第一个参数时Thread*(this)
	// this是前面ThreadRoutine的参数指针（void* arg）
}

void Thread::Join()
{
	pthread_join(threadId_, NULL);
	/* nt pthread_join(pthread_t thread, void **retval);
	 * 用于等待一个线程结束，实现线程间同步
	 * 以阻塞的方式等待threadId执行的线程结束
	 * 当函数返回时，被等待线程的资源被收回
	 * 如果threadId指定的线程已经结束，那么函数会立即返回
	 * **retval存储等待线程的返回值
	 * 返回值：成功返回0，失败返回错误号
	 */
}

void* Thread::ThreadRoutine(void* arg) // 静态函数，不能够调用非静态的成员函数
{
	Thread* thread = static_cast<Thread*>(arg); // 将派生类指针转化为基类指针
	thread->Run(); // 基类指针，但是调用了派生类的成员函数：虚函数的多态
	if (thread->autoDelete_) // 线程是否在执行完毕自动删除
		delete thread;
	return NULL;
}

void Thread::SetAutoDelete(bool autoDelete)
{
	autoDelete_ = autoDelete;
}
