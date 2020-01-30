# C++11线程池

## 已经实现的功能

	1. core个数的核心线程,并根据任务量最大扩展到max个
	2. 可以设置线程数量
	3. 可以判断线程池状态
	4. 可以使用execute和submit提交任务
	5. 可以设置线程回收策略也可以手动回收非core线程

## TODO

	1. 实现interruptible_thread--不使用interruptible_thread
	2. 实现workStealingThreadPool
	3. 实现Executors工厂函数
	4. 实现ScheduledThreadPool 

## 用法类似Java Executor

## 参考
	1. JDK1.8源码
	2. C++并发编程实战

## change log

	1. 实现线程池动态可伸缩--未使用interruptible_thread,利用join()将线程释放后移出线程队列
