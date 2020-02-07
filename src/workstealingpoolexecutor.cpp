#include "workstealingpoolexecutor.hpp"

WorkStealingPoolExecutor::WorkStealingPoolExecutor(int32_t corePoolSize,
        int32_t maxPoolSize,
        std::vector<BlockingQueue<Runnable>>& workQueue,
        const RejectedExecutionHandler& handler,
        const std::string& prefix)
    : ThreadPoolExecutor(corePoolSize, maxPoolSize, workQueue, handler) {}

WorkStealingPoolExecutor::WorkStealingPoolExecutor(int32_t corePoolSize,
        int32_t maxPoolSize,
        std::vector<BlockingQueue<Runnable>>* workQueue,
        RejectedExecutionHandler* handler,
        const std::string& prefix)
    : ThreadPoolExecutor(corePoolSize, maxPoolSize, workQueue, handler, prefix) {}

WorkStealingPoolExecutor::WorkStealingPoolExecutor(int32_t corePoolSize,
        int32_t maxPoolSize,
        const std::string& prefix)
    : ThreadPoolExecutor(corePoolSize, maxPoolSize, prefix) {}

void WorkStealingPoolExecutor::coreWorkerThread(size_t queueIdex) {
    setCurrentThreadName(prefix_);
    Runnable task;
    std::unique_lock<std::mutex> lk(mutex_);
    while(runStateOf(ctl_.load()) <= SHUTDOWN) {
        if (workQueues_->operator[](queueIdex).is_empty() &&
                workQueues_->operator[]((queueIdex + 1) % workQueues_->size()).is_empty()) {
            notEmpty_.wait(lk);
        }
        if(workQueues_->operator[](queueIdex).try_pop(task) ||
                workQueues_->operator[]((queueIdex + 1) % workQueues_->size()).try_pop(task)) {
            task();
        }
    }
}

void WorkStealingPoolExecutor::workerThread(size_t queueIdex) {
    setCurrentThreadName(prefix_);
    Runnable task;
    std::unique_lock<std::mutex> lk(mutex_);
    while(runStateOf(ctl_.load()) <= SHUTDOWN) {
        if (workQueues_->operator[](queueIdex).is_empty() &&
                workQueues_->operator[]((queueIdex + 1) % workQueues_->size()).is_empty()) {
            notEmpty_.wait(lk);
        }
        if(workQueues_->operator[](queueIdex).try_pop(task) ||
                workQueues_->operator[]((queueIdex + 1) % workQueues_->size()).try_pop(task)) {
            task();
        } else {
            if(nonCoreThreadAlive_) {
                std::this_thread::yield();
            } else {
                return;
            }
        }
    }
}
