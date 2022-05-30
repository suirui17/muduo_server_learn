### TimerId

不透明的标识符，用来取消定时器

### Timer

对定时器的一个高层抽象

并没有调用任何定时器相关的函数

### TimerQueue

只有两个对外接口：addTimer和cancel

EventLoop调用的函数

 * runAt：在某个时刻运行定时器

 * runAfter：过一段时间运行定时器

 * runEvery：每隔一段时间运行定时器

 以上三个函数最终都会调用的TimerQueue的addTimer函数

 * cancel：取消定时器

 该函数最终会调用TimerQueue的cancel函数

 TimerQueue的数据结构选择：能快速根据当前时间找到已到期的定时器，也要高效得天剑或删除Timer，因而可以使用二叉搜索树，用map或set

 **TimerQueue可以理解为计时器的序列，以set的方式存储，其中存储了一个timerfd（定时器文件描述符）和定时器通道，该定时器负责在队列中最早的定时器超时时通过channel产生可读事件**



