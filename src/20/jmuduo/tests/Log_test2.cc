#include <muduo/base/Logging.h>
#include <errno.h>
#include <stdio.h>

using namespace muduo;

FILE* g_file;

void dummyOutput(const char* msg, int len)
{
	if (g_file)
	{
		fwrite(msg, 1, len, g_file);
	}
}

void dummyFlush()
{
	fflush(g_file);
}

int main()
{
	g_file = ::fopen("/tmp/muduo_log", "ae");
	Logger::setOutput(dummyOutput);
	Logger::setFlush(dummyFlush);

	LOG_TRACE<<"trace ...";
	// 
	// 

	LOG_DEBUG<<"debug ...";
	LOG_INFO<<"info ...";
	// 即 muduo::Logger(__FILE__, __LINE__).stream() << "info..."
	// 构造一个无名的LogStream对象，并通过LogStream对象输出
	// __FILE__ 该行代码所在的文件名
	// __LINE__ 该行代码所在的行数
	LOG_WARN<<"warn ...";
	LOG_ERROR<<"error ...";
	//LOG_FATAL<<"fatal ...";
	errno = 13;
	LOG_SYSERR<<"syserr ...";
	//LOG_SYSFATAL<<"sysfatal ...";

	::fclose(g_file);

	return 0;
}
