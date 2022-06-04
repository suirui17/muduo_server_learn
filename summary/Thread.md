### 线程标识符

linux中，每一个进程都有一个pid，类型为pid_t，由getpid()获得

linux下的POSIX线程也有一个id，类型是pthread_t，由pthread_self()获得，该id由线程库维护，其id空间是各个进程独立的；linux中的POSIX线程库实现的线程其实也是一个进程LWP，只是该进程与主进程共享一些资源（如代码段和数据段等）

_POSIX（可移植操作系统接口）线程是一种支持内存共享的简单工具，是提高代码相应和性能的有力手段_

有时候需要线程的真实id，既不是进程的pid，也不是线程的pthread_id，而是线程的真实id，称为tid，可以通过gettid()来获得

### 线程相关函数

```c
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
```
pthread_t *thread：传递一个pthread_t变量地址进来,用于保存新线程的tid(线程ID)

const pthread_attr_t *attr：线程属性设置,如使用默认属性,则传NULL

void *(*start_routine) (void *)： 函数指针,指向新线程应该加载执行的函数模块，即**函数入口地址**

void *arg：指定线程将要加载调用的那个函数的参数

```c
int pthread_join(pthread_t thread, void **retval);
```
pthread_t thread：回收线程的tid

void **retval：接收退出线程传递出的返回值

调用该函数的线程将挂起等待，直到id为thread的线程终止

```c
void
 pthread_exit(void *retval)
```
void *retval： 线程退出时传递出的参数,可以是退出值或地址,如是地址时,不能是线程内部申请的局部地址

```c
int pthread_cancel(pthread_t thread);
```

```c
int pthread_atfork(void(*prepare(void)), void (*parent)(void), void(*child)(void));
```

调用fork时，内部创建子进程前在父进程中会调用prepare，内部创建子进程成功后，父进程会调用parent，子进程会调用child，父进程和子进程调用相应函数之后会返回


### __thread

__thread修饰的变量是线程局部存储的，线程之间不共享，为每个线程私有

只能修改POD类型，即与c兼容的原始数据，例如结构和整型等，带有用户定义的构造函数或虚函数的类则不是

TSD：线程特定数据，可以用于非POD类型