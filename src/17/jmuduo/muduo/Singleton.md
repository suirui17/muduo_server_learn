### atexit()


```c
int atexit(void (*func)(void));
```

当程序正常终止时，调用指定的func函数，可以在任何地方注册终止函数，但其只会在程序终止的时候被调用