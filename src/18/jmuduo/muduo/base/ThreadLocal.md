### TSD

Thread-specific Data 线程特定数据，也可以称为线程本地存储TLS Thread-local storage

POSIX线程库通过以下函数操作线程特定数据

```c
int pthread_key_create(pthread_key_t *key, void (*destructor)(void*));
```
第一个参数为指向一个键值的指针，第二个参数指明一个destructor函数

如果第二个参数不为空，那么每个线程结束时，系统将调用该函数来释放绑定在这个键上的内存块，如果为null，系统将调用默认的清理函数

key一旦被创建，所有线程都可以访问它，但各线程可根据自己的需要往key中填入不同的值，这就相当于提供了一个同名而不同值的全局变量，一键多值

一键多值靠的是一个关键数据结构数组，即TSD池，创建一个TSD就相当于结构数组中某一项设置为in_use，并将其索引返回给*key，然后设置清理函数

```c
int pthread_key_delete(pthread_key_t key);
```

删除一个键，删除后，键所占的内存将被释放，注销一个TSD

该函数并不检查当前是否有线程占用TSD，也不会调用清理函数

键占用的内存被释放，与键关联的线程数据所占的内存并不会被释放，因此线程数据的释放，必须在释放键之前完成

```c
void *pthread_key_getspecific(pthread_key_t key);
```

```c
int pthread_key_setspecific(pthread_key_t key,const void *pointer));
```




