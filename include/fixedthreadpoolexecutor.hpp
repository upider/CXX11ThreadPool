#ifndef FIXEDTHREADPOOLEXECUTOR_H
#define FIXEDTHREADPOOLEXECUTOR_H

#include <adjustablethreadpoolexecutor.hpp>

class FixedThreadPoolExecutor {
    public:
        FixedThreadPoolExecutor();
        virtual ~FixedThreadPoolExecutor ();

    public:

    private:
        int											  _corePoolSize;
        size_t										  _threadNum;
        mutable std::mutex                            _mutex;
        std::atomic_int32_t                           _ctl;
        std::vector<std::thread>                      _threads;
        std::unique_ptr<RejectedExecutionHandler>	  _handler;
        /// runState is stored in the high-order bits
        /// 我们可以看出有5种runState状态，证明至少需要3位来表示runState状态
        /// 所以高三位就是表示runState了
        static const int32_t						  COUNT_BITS;
        static const int32_t						  CAPACITY;
        static const int32_t                          RUNNING;        ///运行状态
        static const int32_t                          SHUTDOWN;       ///不再接受新任务
        static const int32_t                          STOP;           ///不再接受新任务,清空任务队列
        static const int32_t                          TIDYING;        ///所有线程已经释放,任务队列为空,会调用terminated()
        static const int32_t                          TERMINATED;     ///线程池关闭,terminated()函数已经执行
};

#endif /* FIXEDTHREADPOOLEXECUTOR_H */
