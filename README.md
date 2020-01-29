# C++11线程池

## 已经实现的功能

	1. core个数的核心线程,并根据任务量最大扩展到max个
	2. 可以设置线程数量
	3. 可以判断线程池状态
	4. 可以使用execute和submit提交任务

## TODO

	1. 实现interruptible_thread
	2. 利用interruptible_thread实现线程池动态可伸缩
	3. 实现workStealingThreadPool
	4. 实现Executors工厂函数

## 用法类似Java Executor

## 参考
	1. JDK1.8源码
	2. C++并发编程实战
