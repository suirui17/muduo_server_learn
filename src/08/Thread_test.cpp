#include "Thread.h"
#include <unistd.h>
#include <iostream>
using namespace std;

class TestThread : public Thread
{
public:
	TestThread(int count) : count_(count)
	{
		cout<<"TestThread ..."<<endl;
	}

	~TestThread()
	{
		cout<<"~TestThread ..."<<endl;
	}

private:
	void Run()
	{
		while (count_--)
		{
			cout<<"this is a test ..."<<endl;
			sleep(1);
		}
	}

	int count_;
};

int main(void)
{
	/*
	TestThread t(5);
	t.Start();

	t.Join();
	// 至此t2线程已经执行完毕，但是t2对象仍然存在，即线程对象的生命周期和线程的生命周期是不一样的
	*/

	TestThread* t2 = new TestThread(5);
	t2->SetAutoDelete(true);
	t2->Start();
	t2->Join();
	// 线程对象自动销毁：动态创建对象
	for (; ; )
		pause();

	return 0;
}
