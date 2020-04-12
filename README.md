# C++11线程池

## 已经实现的功能

	1. corePoolSize个数的核心线程(核心线程不会退出),并根据任务量最大扩展到maxPoolSize个
	2. 线程池线程数=corePoolSize+提交的线程数量(submit和execute),不会超过maxPoolSize
	3. 可以设置线程数量
	4. 可以判断线程池状态
	6. 可以使用execute和submit提交任务(两种方式有差别)
	7. 可以设置线程回收策略也可以手动回收非core线程
	8. 线程回收策略使用的是thread.join(),可以避免线程资源无法释放
	9. everPoolSize是线程池出现过的线程数
	10. 每个线程有自己的工作队列
	11. Thread类丰富了std::thread的功能,设置名称,id,检查是否存活,是否空闲等等
	12. 任务窃取线程池
	13. 定时调用线程池,任务队列采用vector形成小顶堆,时间最小排在前面
	14. Thread类不需要手动释放(join或detach)

## 用法
	1. 类似Java Executor
	2. 例子:example文件夹下
	3. 符合C++11标准
	4. linux平台(可能会做跨平台)

	5. Thread示例

	```c
	std::promise<int> promise;
    std::future<int> future(promise.get_future());

    Thread t([&promise]() {
        std::cout << "thread start.." << std::endl;
        promise.set_value(999);
    });

    t.start();
	//不需要手动释放线程资源
    //t.join();

    std::cout << future.get() << std::endl;
	```

	6. ScheduledThreadPoolExecutor示例

	```c
    ScheduledThreadPoolExecutor tpe(3, "STPE");
    tpe.preStartCoreThreads();
	//在ScheduledThreadPoolExecutor中使用std::future
    std::packaged_task<std::string()> p([]() ->std::string {
        std::ostringstream os;
        std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cout << "Task: " << std::asctime(std::localtime(&tt));
        os << "Task schedule's the last time is " << std::asctime(std::localtime(&tt));
        return os.str();
    });
    std::future<std::string> f(p.get_future());
    tpe.scheduleAtFixedRate([&]() {
        p();
        p.reset();//每次执行后都要取消关联才能再次执行
    }, std::chrono::seconds(2), std::chrono::seconds(2));
	```

## 缺陷
	1. 线程使用的任务队列有锁
	2. 所有线程对任务执行的包装是一个Runnable类，对象构造比较耗时，可以改为std::function
	3. 定时调用队列使用了sleep，会使线程睡眠，之后唤醒，切换耗时较大，可以使用epoll
	4. 不能限制任务数量（限流）
	不建议在生产环境使用

## 参考
	1. JDK1.8源码
	2. C++并发编程实战
	3. folly

## change log

	1. 实现线程池动态可伸缩--未使用interruptible_thread,利用join()将线程释放后移出线程队列
	2. 实现Thread类--不使用interruptible_thread,Thread类通过重写run()可以实现想要的操作
	3. 完善了ThreadPoolExecutor,每个线程有自己的工作队列,可以批量添加任务
	4. interruptible_thread功能可能会考虑以后添加
	5. 实现WorkStealingThreadPool
	6. 修改BlockingQueue<Runable>为BlockingQueue<Runable::sptr>,这样任务提交后仍然能够拿到结果
	7. Runnable类的复制构造函数不会复制原来Runnable对象初始化的lambda
	8. 实现ScheduledThreadPoolExecutor

## License

	Mozilla
