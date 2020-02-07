#include "workstealingpoolexecutor.hpp"

WorkStealingPoolExecutor::WorkStealingPoolExecutor(int32_t corePoolSize,
        int32_t maxPoolSize,
        const std::vector<BlockingQueue<Runnable::sptr>>& workQueue,
        const RejectedExecutionHandler& handler,
        const std::string& prefix)
    : ThreadPoolExecutor(corePoolSize, maxPoolSize, workQueue, handler) {}

WorkStealingPoolExecutor::WorkStealingPoolExecutor(int32_t corePoolSize,
        int32_t maxPoolSize,
        const std::vector<BlockingQueue<Runnable::sptr>>& workQueue,
        RejectedExecutionHandler* handler,
        const std::string& prefix)
    : ThreadPoolExecutor(corePoolSize, maxPoolSize, workQueue, handler, prefix) {}

WorkStealingPoolExecutor::WorkStealingPoolExecutor(int32_t corePoolSize,
        int32_t maxPoolSize,
        const std::string& prefix)
    : ThreadPoolExecutor(corePoolSize, maxPoolSize, prefix) {}

void WorkStealingPoolExecutor::coreWorkerThread(size_t queueIdex) {
    setCurrentThreadName(prefix_);
    Runnable::sptr task;
    std::unique_lock<std::mutex> lk(mutex_);
    while(runStateOf(ctl_.load()) <= SHUTDOWN) {
        if (workQueues_[queueIdex].is_empty() &&
                workQueues_[(queueIdex + 1) % workQueues_.size()].is_empty()) {
            notEmpty_.wait(lk);
        }
        if(workQueues_[queueIdex].try_pop(task) ||
                workQueues_[(queueIdex + 1) % workQueues_.size()].try_pop(task)) {
            task->operator()();
        }
    }
}

void WorkStealingPoolExecutor::workerThread(size_t queueIdex) {
    setCurrentThreadName(prefix_);
    Runnable::sptr task;
    std::unique_lock<std::mutex> lk(mutex_);
    while(runStateOf(ctl_.load()) <= SHUTDOWN) {
        if (workQueues_[queueIdex].is_empty() &&
                workQueues_[(queueIdex + 1) % workQueues_.size()].is_empty()) {
            notEmpty_.wait(lk);
        }
        if(workQueues_[queueIdex].try_pop(task) ||
                workQueues_[(queueIdex + 1) % workQueues_.size()].try_pop(task)) {
            task->operator()();
        } else {
            if(nonCoreThreadAlive_) {
                std::this_thread::yield();
            } else {
                return;
            }
        }
    }
}
