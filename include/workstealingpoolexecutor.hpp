#ifndef WORKSTEALINGPOOLEXECUTOR_H
#define WORKSTEALINGPOOLEXECUTOR_H

#include "threadpoolexecutor.hpp"

class WorkStealingPoolExecutor: public ThreadPoolExecutor {
    public:
        WorkStealingPoolExecutor(int32_t corePoolSize,
                                 int32_t maxPoolSize,
                                 std::vector<BlockingQueue<Runnable>>& workQueue,
                                 const RejectedExecutionHandler& handler,
                                 const std::string& prefix = "");

        WorkStealingPoolExecutor(int32_t corePoolSize,
                                 int32_t maxPoolSize,
                                 std::vector<BlockingQueue<Runnable>>* workQueue,
                                 RejectedExecutionHandler* handler,
                                 const std::string& prefix = "");

        WorkStealingPoolExecutor(int32_t corePoolSize,
                                 int32_t maxPoolSize,
                                 const std::string& prefix = "");

        ~WorkStealingPoolExecutor() {}

    public:
        /**
         * @brief submit 在将来某个时候执行给定的任务,
         *               任务可以在新线程或现有的合并的线程中执行,
         *               可以有返回值,向任务队列提交的是任务副本
         *               会抛出异常
         *
         * @param f 要提交的任务(Runnable或函数或lambda)
         * @param core 是否使用核心线程(默认值true,不增加新线程)
         *
         * @return res 任务返回值的future
         */
        template<typename F>
        std::future<typename std::result_of<F()>::type>
        submit(F f, bool core = true) {
            using result_type = typename std::result_of<F()>::type;
            std::packaged_task<result_type()> task(std::move(f));
            std::future<result_type> res(task.get_future());
            if(addWorker(std::move(task), core)) {
                return res;
            } else {
                int c = ctl_.load();
                if (!isRunning(c)) {
                    reject(std::move(task));
                    return res;
                }
            }
        }

        virtual void workerThread(size_t queueIdex) override;

        virtual void coreWorkerThread(size_t queueIdex) override;
};

#endif /* WORKSTEALINGPOOLEXECUTOR_H */
