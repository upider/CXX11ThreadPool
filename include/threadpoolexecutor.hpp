#ifndef THREADPOOLEXECUTOR_HPP
#define THREADPOOLEXECUTOR_HPP

#include <future>
#include <vector>

#include "thread.hpp"
#include "blockingqueue.hpp"
#include "runnable.hpp"

/**
 * @brief 不再接受任务时的拒绝策略
 */
class RejectedExecutionHandler {
    public:
        /**
         * @brief RejectedExecutionHandler 构造函数
         */
        RejectedExecutionHandler() = default;
        /**
         * @brief ~RejectedExecutionHandler 析构函数
         */
        virtual ~RejectedExecutionHandler() = default;

    public:
        /**
         * @brief 拒绝策略的执行函数
         */
        virtual void rejectedExecution(const Runnable::sptr r) {
            throw std::logic_error("thread pool is not RUNNING");
        }

        /**
         * @brief 拒绝策略的执行函数
         */
        virtual void rejectedExecution(const Runnable& r) {
            throw std::logic_error("thread pool is not RUNNING");
        }
};

/**
 * @brief 线程池基本实现,每个线程都有一个任务队列
 */
class ThreadPoolExecutor {
    public:
        /**
         * @brief ThreadPoolExecutor 构造函数,workQueue的大小要不大于corePoolSize
         *							 每个线程根据顺序将对应位置的BlockingQueue<Runnable>
         *							 作为自己的任务队列
         *                           会抛出异常
         *
         * @param corePoolSize 核心线程数
         * @param maxPoolSize 最大线程数
         * @param workQueue 任务队列
         * @param handler 拒绝任务句柄
         * @param prefix  线程名前缀
         */
        explicit ThreadPoolExecutor(int32_t corePoolSize,
                                    int32_t maxPoolSize,
                                    const std::vector<BlockingQueue<Runnable::sptr>>& workQueue,
                                    const RejectedExecutionHandler& handler,
                                    const std::string& prefix = ""
                                   );

        /**
         * @brief ThreadPoolExecutor 构造函数,workQueue的大小要不大于corePoolSize
         *							 每个线程根据顺序将对应位置的BlockingQueue<Runnable>
         *							 作为自己的任务队列
         *                           会抛出异常
         *
         * @param corePoolSize 核心线程数
         * @param maxPoolSize 最大线程数
         * @param workQueue 任务队列
         * @param handler 拒绝任务句柄
         * @param prefix 线程名前缀
         */
        explicit ThreadPoolExecutor(int32_t corePoolSize,
                                    int32_t maxPoolSize,
                                    const std::vector<BlockingQueue<Runnable::sptr>>& workQueue,
                                    RejectedExecutionHandler* handler,
                                    const std::string& prefix = ""
                                   );

        /**
         * @brief ThreadPoolExecutor 构造函数,根据corePoolSize构造相同大小的workQueue
         *							 每个线程根据顺序将对应位置的BlockingQueue<Runnable>
         *							 作为自己的任务队列
         *                           会抛出异常
         *
         * @param corePoolSize 核心线程数
         * @param maxPoolSize 最大线程数
         * @param prefix  线程名前缀
         */
        explicit ThreadPoolExecutor(int32_t corePoolSize,
                                    int32_t maxPoolSize,
                                    const std::string& prefix = "");

        /**
         * @brief ~ThreadPoolExecutor 析构函数
         */
        virtual ~ThreadPoolExecutor ();

    public:
        /**
         * @brief nonCoreThreadAlive 是否允许非核心线程超时,若允许则线程执行完
         *                             一个任务后不会退出,会继续存在
         *
         * @return ture - 允许非核心thread超时
         */
        virtual bool keepNonCoreThreadAlive () const final;

        /**
         * @brief keepNonCoreThreadAlive 设置是否允许非核心thread超时并且在没有任务时终止
         *
         * @param value ture或false
         */
        virtual void keepNonCoreThreadAlive(bool value) final;

        /**
         * @brief releaseNonCoreThreads 释放非核心线程(释放线程资源,并弹出线程队列)
         *								如果keepNonCoreThreadAlive=false,
         *								那么非核心线程会自动退出
         *								执行时会将keepNonCoreThreadAlive设为false,
         *								释放所有非核心线程
         */
        virtual void releaseNonCoreThreads();

        /**
         * @brief setRejectedExecutionHandler 设置新的任务拒绝策略
         *
         * @param handler RejectedExecutionHandler类对象
         */
        virtual void setRejectedExecutionHandler(RejectedExecutionHandler handler) final;

        /**
            * @brief execute 在将来某个时候执行给定的任务,无返回值,
            *                任务可以在新线程或现有的合并的线程中执行,
            *                向任务队列提交的是任务副本
            *                不会抛出异常
            *
            * @param command 要执行的任务(Runnable的子类shared_ptr),
        	*                任务执行完依然能够拿到结果
            * @param core    是否使用核心线程,在核心线程没有完全启动时,
        	*                会忽略core的值,优先启动核心线程
            *
            * @return          true - 添加成功
            */
        bool execute(Runnable::sptr command, bool core = true);

        /**
         * @brief execute 在将来某个时候执行给定的任务,无返回值,
         *                任务可以在新线程或现有的合并的线程中执行,
         *                向任务队列提交的是任务副本
         *                不会抛出异常
         *
         * @param command 要执行的任务(Runnable或函数或lambda),任务会被用std::move(转移),
         *                任务结束后就会消失
         *
         * @param core    是否使用核心线程,在核心线程没有完全启动时,
         *                会忽略core的值,优先启动核心线程
         *
         * @return          true - 添加成功
         */
        virtual bool execute(Runnable& command, bool core = true);

        /**
         * @brief execute 在将来某个时候执行给定的任务,无返回值,
         *                任务可以在新线程或现有的合并的线程中执行,
         *                向任务队列提交的是任务副本
         *                不会抛出异常
         *
         * @param commands 要执行的任务队列
         * @param core 是否使用核心线程,如果为true,任务将被平均分配给核心线程
         *             如果为flase,新建线程执行任务队列(前提是线程池小于maxPoolSize)
         *
         * @return true - 任务全部放入执行队列
         */
        virtual bool execute(BlockingQueue<Runnable::sptr>& commands, bool core = true);

        /**
         * @brief submit 在将来某个时候执行给定的任务,
         *               任务可以在新线程或现有的合并的线程中执行,
         *               可以有返回值,向任务队列提交的是任务副本
         *               会抛出异常
         *
         * @param f 要提交的任务(Runnable或函数或lambda)
         * @param core 是否使用核心线程(默认值true,不增加新线程),
         *             在核心线程没有完全启动时,会忽略core的值,优先启动核心线程
         *
         * @return res 任务返回值的future
         */
        template<typename F>
        std::future<typename std::result_of<F()>::type>
        submit(F f, bool core = true) {
            using result_type = typename std::result_of<F()>::type;
            std::packaged_task<result_type()> task(std::move(f));
            std::future<result_type> res(task.get_future());
            if(addWorker(Runnable(std::move(task)), core)) {
                return res;
            } else {
                int c = ctl_.load();
                if (!isRunning(c)) {
                    reject(Runnable(std::move(task)));
                    return res;
                }
            }
        }

        /**
         * @brief toString 返回标识此池的字符串及其状态，包括运行状态和估计的Worker和任务计数的指示
         *
         * @return 一个标识这个池的字符串，以及它的状态
         */
        virtual std::string toString() const;

        /**
         * @brief getActiveCount 返回正在执行任务的线程的大概数量
         *                       也就是非空闲线程数量
         *
         * @return 线程数
         */
        virtual int getActiveCount() const final;

        /**
         * @brief getTaskCount 得到任务队列大小
         *
         * @return 任务队列大小
         */
        virtual long getTaskCount() const;

        /**
         * @brief setMaxPoolSize 设置允许的最大线程数。
         *                       这将覆盖在构造函数中设置的任何值。
         *                       如果新值小于当前值，则过多的现有线程在下一个空闲时将被终止
         *
         * @param maxPoolSize 新的最大值
         */
        virtual void setMaxPoolSize(int maxPoolSize) final;

        /**
         * @brief getLargestPoolSize 返回池中使用过的线程数
         *
         * @return 线程数
         */
        virtual int getEverPoolSize() const final;

        /**
         * @brief getCorePoolSize 返回核心线程数
         *
         * @return 核心线程数
         */
        virtual int getCorePoolSize() const final;

        /**
         * @brief shutdown 不在接受新任务,并且在所有任务执行完后终止线程池
         *                 如果还有新任务提交将会触发拒绝策略,默认的拒绝策略会抛出异常
         */
        virtual void shutdown() final;

        /**
         * @brief stop 不在接受新任务,终止旧任务,释放正在运行的线程
         */
        virtual void stop() final;

        /**
         * @brief isShutDown 判断线程池是否shutdown
         *
         * @return
         */
        virtual bool isShutDown() final;

        /**
         * @brief isTerminated 判断线程池是否terminated
         *
         * @return
         */
        virtual bool isTerminated() final;

        /**
         * @brief preStartCoreThreads 启动所有核心线程，导致他们等待工作。
         *                         这将覆盖仅在执行新任务时启动核心线程的默认策略
         *
         * @return 线程数已启动
         */
        virtual int preStartCoreThreads();

        /**
         * @brief reject 将任务抛弃
         *
         * @param command 要抛弃的任务
         */
        virtual inline void reject(const Runnable & command) final {
            rejectHandler_->rejectedExecution(command);
        }

    protected:
        /**
         * @brief terminated 线程池终止时执行
         */
        virtual void terminated() {}

    protected:
        /**
         * @brief releaseWorkers 释放所有线程(释放线程资源,并弹出线程队列)
         */
        virtual void releaseWorkers();

        /**
         * @brief runStateOf 得到线程池状态
         *
         * @param c 控制变量
         *
         * @return 运行状态
         */
        virtual inline int runStateOf(int32_t c) const final {
            return c & ~CAPACITY;
        }

        /**
         * @brief workerCountOf 得到工作线程数量
         *
         * @param c 控制变量
         *
         * @return 工作线程数量
         */
        virtual inline int workerCountOf(int32_t c) const final {
            return c & CAPACITY;
        }

        /**
         * @brief ctlOf 控制变量初始化
         *
         * @param rs 运行状态
         * @param wc 工作线程数量
         *
         * @return 控制变量的值
         */
        virtual inline int32_t ctlOf(int32_t rs, int32_t wc) const final {
            return rs | wc;
        }

        /**
         * @brief addWorker 将任务添加到队列
         *
         * @param task 任务
         * @param core      是否使用核心线程
         *
         * @return          true - 添加成功
         */
        virtual bool addWorker(Runnable task, bool core = true);

        /**
            * @brief addWorker 将任务添加到队列
            *
            * @param task 任务
            * @param core      是否使用核心线程
            *
            * @return          true - 添加成功
            */
        virtual bool addWorker(Runnable::sptr task, bool core = true);

        /**
         * @brief advanceRunState 改变线程池状态
         *
         * @param targetState 目标状态
         */
        virtual void advanceRunState(int32_t targetState) final;

        /**
         * @brief coreWorkerThread 核心线程循环
         *
         * @param queueIdex 任务队列位置
         */
        virtual void coreWorkerThread(size_t queueIdex);

        /**
         * @brief workerThread 非核心线程循环
         *
         * @param queueIdex 任务队列位置
         */
        virtual void workerThread(size_t queueIdex);

        /**
         * @brief reject 将任务抛弃
         *
         * @param command 要抛弃的任务
         */
        virtual inline void reject(const Runnable::sptr command) final {
            rejectHandler_->rejectedExecution(command);
        }

        /**
         * @brief isRunning 是否还在运行
         *
         * @param c
         *
         * @return
         */
        virtual inline bool isRunning(int c) const final {
            return c < SHUTDOWN;
        }

        /**
         * @brief runStateLessThan 状态低于
         *
         * @param c 控制变量
         * @param s 比较对象
         *
         * @return bool c < s
         */
        virtual inline bool runStateLessThan(int c, int s)const final {
            return c < s;
        }

        /**
         * @brief runStateAtLeast 运行状态最少是
         *
         * @param c 控制变量
         * @param s 比较对象
         *
         * @return bool c < s
         */
        virtual inline bool runStateAtLeast(int c, int s)const final {
            return c >= s;
        }

        /**
         * @brief compareAndIncrementWorkerCount 尝试CAS递增ctl的workerCount字段.
         *
         * @param expect 控制变量
         *
         * @return true-成功
         */
        virtual bool compareAndIncrementWorkerCount(int expect) final {
            return ctl_.compare_exchange_strong(expect, expect + 1);
        }

        /**
         * @brief compareAndDecrementWorkerCount 尝试CAS递减ctl的workerCount字段.
         *
         * @param expect 控制变量
         *
         * @return bool
        */
        virtual bool compareAndDecrementWorkerCount(int expect)final {
            return ctl_.compare_exchange_weak(expect, expect - 1);
        }

        /**
         *   @brief decrementWorkerCount 减少ctl的workerCount字段
         *
        */
        virtual void decrementWorkerCount() {
            do {

            } while (!compareAndDecrementWorkerCount(ctl_.load()));
        }

    protected:
        ///初始化使用
        const int32_t COUNT_BITS = 29;
        ///线程池容量
        const int32_t CAPACITY   = (1 << COUNT_BITS) - 1;
        ///处于运行状态,RUNNING一定要初始化在ctl_之前,因为crl_用其初始化
        const int32_t RUNNING    = (-1 << COUNT_BITS);
        ///不再接受新任务
        const int32_t SHUTDOWN   = 0 << COUNT_BITS;
        ///不再接受新任务,队列中的任务将被抛弃
        const int32_t STOP       = 1 << COUNT_BITS;
        ///所有线程已经释放,任务队列为空,会调用terminated()
        const int32_t TIDYING    = 2 << COUNT_BITS;
        ///线程池关闭,terminated()函数已经执行
        const int32_t TERMINATED = 3 << COUNT_BITS;

        ///核心线程数
        int											                 corePoolSize_;
        ///最大线程数
        int											                 maxPoolSize_;
        ///提交任务的id
        long long                                                    submitId_{0};
        ///线程名前缀
        std::string                                                  prefix_;
        ///是否允许非核心线程超时
        volatile bool                                                keepNonCoreThreadAlive_{false};
        ///曾经出现的线程数量,包括已经死亡的
        std::atomic<int>                                             everPoolSize_{0};
        ///队列锁
        mutable std::mutex                                           mutex_;
        ///控制变量
        std::atomic_int32_t                                          ctl_;
        ///队列非空条件变量
        std::condition_variable                                      notEmpty_;
        ///线程队列
        std::vector<Thread::sptr>                                    threads_;
        ///任务队列
        std::vector<BlockingQueue<Runnable::sptr>>	                 workQueues_;
        ///拒绝策略回调
        std::unique_ptr<RejectedExecutionHandler>	                 rejectHandler_;

};

#endif /* THREADPOOLEXECUTOR_HPP */
