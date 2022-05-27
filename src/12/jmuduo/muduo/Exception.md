### backtrace()

```c++
backtrace(void **buffer, int size)；
char **backtrace_symbols (void *const *buffer, int size);
void backtrace_symbols_fd (void *const *buffer, int size, int fd);
```

* backtrace()：获取函数调用的堆栈信息，即回溯函数调用表，将数据存储在buffer中

* backtrace_symbols()：将从backtrace()函数获取的地址转化为描述这些地址的字符串数组，每个地址的字符串信息包含对应函数的名字、函数内的十六进制偏移地址、以及实际的返回地址（十六进制）

* backtrace_symbols_fd()：与backtrace_symbols()函数具有相同的功能，不同的是它不会给调用者返回字符串数组，而是将结果写入文件描述符fd的文件中，每个函数对应一行，它不会调用malloc函数，因此可以应用在函数调用可能失败的情况下



