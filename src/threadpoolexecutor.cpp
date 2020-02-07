#include <sstream>
#include <stdexcept>
#include <pthread.h>
#include <signal.h>
#include <iostream>
#include <bitset>

#include "threadpoolexecutor.hpp"

ThreadPoolExecutor::ThreadPoolExecutor(int32_t corePoolSize,
                                       int32_t maxPoolSize,
                                       std::vector<BlockingQueue<Runnable>>& workQueue,
                                       const RejectedExecutionHandler& handler,
                                       const std::string& prefix
                                      )
    : corePoolSize_(corePoolSize),
      maxPoolSize_(maxPoolSize),
      prefix_(prefix),
      ctl_(ctlOf(RUNNING, 0)) {

    if (corePoolSize < 0               ||
            maxPoolSize <= 0           ||
            maxPoolSize < corePoolSize)
        throw std::logic_error("parameter value is wrong");

    this->workQueues_.reset(new std::vector<BlockingQueue<Runnable>>(workQueue));
    this->rejectHandler_.reset(new RejectedExecutionHandler(handler));
    startCoreThreads();
}

ThreadPoolExecutor::ThreadPoolExecutor(int32_t corePoolSize,
                                       int32_t maxPoolSize,
                                       std::vector<BlockingQueue<Runnable>>* workQueue,
                                       RejectedExecutionHandler* handler,
                                       const std::string& prefix
                                      )
    : corePoolSize_(corePoolSize),
      maxPoolSize_(maxPoolSize),
      prefix_(prefix),
      ctl_(ctlOf(RUNNING, 0)) {

    if (corePoolSize < 0               ||
            maxPoolSize <= 0           ||
            maxPoolSize < corePoolSize)
        throw std::logic_error("parameter value is wrong");

    this->workQueues_.reset(workQueue);
    this->rejectHandler_.reset(handler);
    startCoreThreads();
}

ThreadPoolExecutor::ThreadPoolExecutor(int32_t corePoolSize,
                                       int32_t maxPoolSize,
                                       const std::string& prefix
                                      )
    : corePoolSize_(corePoolSize),
      maxPoolSize_(maxPoolSize),
      prefix_(prefix),
      ctl_(ctlOf(RUNNING, 0)) {

    if (corePoolSize < 0               ||
            maxPoolSize <= 0           ||
            maxPoolSize < corePoolSize)
        throw std::logic_error("parameter value is wrong");

    this->workQueues_.reset(new std::vector<BlockingQueue<Runnable>>(corePoolSize));
    this->rejectHandler_.reset(new RejectedExecutionHandler);
    startCoreThreads();
}

ThreadPoolExecutor::~ThreadPoolExecutor() {
    for (auto t : threads_) {
        if (t->joinable()) {
            t->join();
        }
    }
}

bool ThreadPoolExecutor::keepNonCoreThreadAlive() const {
    return nonCoreThreadAlive_;
}

void ThreadPoolExecutor::keepNonCoreThreadAlive(bool value) {
    if (value != nonCoreThreadAlive_) {
        nonCoreThreadAlive_ = value;
    }
    if (value) {
        releaseNonCoreThreads();
    }
}

void ThreadPoolExecutor::releaseNonCoreThreads(bool onlyOne) {
    nonCoreThreadAlive_ = false;
    notEmpty_.notify_all();
    for (int i = threads_.size() - 1; i >= corePoolSize_; --i) {
        if (threads_[i] != nullptr) {
            if (threads_[i]->joinable()) {
                threads_[i]->join();
                decrementWorkerCount();
                threads_.pop_back();
                workQueues_->pop_back();
            }
        }
        if (onlyOne)
            break;
    }
    threads_.shrink_to_fit();
    nonCoreThreadAlive_ = true;
}

void ThreadPoolExecutor::releaseWorkers() {
    notEmpty_.notify_all();
    for (int i = threads_.size() - 1; i >= 0 ; --i) {
        if (threads_[i]->joinable()) {
            threads_[i]->join();
            decrementWorkerCount();
            threads_.pop_back();
        }
    }
}

bool ThreadPoolExecutor::addWorker(const Runnable firstTask, bool core) {
    int32_t c = 0;
    int32_t rs = 0;
    int32_t wc = 0;
    for (;;) {
        c = ctl_.load();
        rs = runStateOf(c);
        if (rs >= SHUTDOWN)
            return false;
        for (;;) {
            if (core) {
                workQueues_->operator[](submitId_++ % corePoolSize_).put(std::move(firstTask));
                notEmpty_.notify_one();
                return true;
            }
            wc = workerCountOf(c);
            if (wc >=  maxPoolSize_) {
                workQueues_->operator[](submitId_++ % workQueues_->size()).put(std::move(firstTask));
                notEmpty_.notify_one();
                return true;
            }
            if(compareAndIncrementWorkerCount(c)) {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    workQueues_->push_back(BlockingQueue<Runnable>());
                    workQueues_->operator[](workQueues_->size() - 1).put(std::move(firstTask));
                }
                threads_.emplace_back(new Thread(std::bind(&ThreadPoolExecutor::workerThread,
                                                 this, workQueues_->size() - 1)));
                threads_[threads_.size() - 1]->start();
                everPoolSize_++;
                notEmpty_.notify_one();
                return true;
            }
            c = ctl_.load();  // Re-read ctl
            if(runStateOf(c) != rs)
                break;
        }
    }
}

bool ThreadPoolExecutor::execute(const Runnable& command, bool core) {
    int32_t c = ctl_.load();
    if(isRunning(c)) {
        if(addWorker(command, core)) {
            return true;
        }
        c = ctl_.load();
        if (!isRunning(c)) {
            reject(command);
            return false;
        }
    }
    return false;
}

bool ThreadPoolExecutor::execute(BlockingQueue<Runnable>& commands, bool core) {
    int32_t c = ctl_.load();
    if (core) {
        for (int i = 0; i < commands.size(); ++i) {
            c = ctl_.load();
            if(isRunning(c)) {
                auto command = commands.take();
                if(addWorker(command, core)) {
                    return true;
                }
                c = ctl_.load();
                if (!isRunning(c)) {
                    reject(command);
                    return false;
                }
            } else {
                return false;
            }
        }
    } else {
        c = ctl_.load();
        int wc = workerCountOf(c);
        if (wc >= CAPACITY || wc >= maxPoolSize_)
            return false;
        if (compareAndIncrementWorkerCount(c)) {
            std::lock_guard<std::mutex> lock(mutex_);
            workQueues_->push_back(commands);
            threads_.emplace_back(new Thread(std::bind(&ThreadPoolExecutor::workerThread, this, workQueues_->size() - 1)));
            threads_[threads_.size() - 1]->start();
            everPoolSize_++;
        }
        notEmpty_.notify_one();
    }
    return true;
}

void ThreadPoolExecutor::setRejectedExecutionHandler(RejectedExecutionHandler handler) {
    rejectHandler_.reset(new RejectedExecutionHandler(handler));
}

std::string ThreadPoolExecutor::toString() const {
    int32_t c = ctl_.load();
    std::string rs = (runStateLessThan(c, SHUTDOWN) ? "Running" :
                      (runStateAtLeast(c, TERMINATED) ? "Terminated" :
                       "ShuttingDown"));
    std::stringstream ss;
    ss << "STATE="               << rs
       << " EVER_POOL_SIZE="     << everPoolSize_
       << " CORE_POOL_SIZE="     << corePoolSize_
       << " MAX_POOL_SIZE="      << maxPoolSize_
       << " TASK_QUEUE_SIZE="    << getTaskCount();
    return ss.str();
}

int ThreadPoolExecutor::getActiveCount() const {
    int count = 0;
    for (auto& e : threads_) {
        if(!e->isIdle())
            count++;
    }
    return count;
}

long ThreadPoolExecutor::getTaskCount()const {
    std::lock_guard<std::mutex> lock(mutex_);
    long size = 0;
    for (auto e : *workQueues_) {
        size += e.size();
    }
    return size;
}

void ThreadPoolExecutor::setMaxPoolSize(int32_t maxPoolSize) {
    if (maxPoolSize <= 0 || maxPoolSize < corePoolSize_)
        return;
    this->maxPoolSize_ = maxPoolSize;
    if (workerCountOf(ctl_.load()) > maxPoolSize_) {
        releaseNonCoreThreads();
    }
}

int ThreadPoolExecutor::getEverPoolSize() const {
    return everPoolSize_.load();
}

int ThreadPoolExecutor::startCoreThreads() {
    int32_t c = ctl_.load();
    for (int i = 0; i < corePoolSize_; ++i) {
        if (compareAndIncrementWorkerCount(c)) {
            threads_.emplace_back(new Thread(std::bind(&ThreadPoolExecutor::coreWorkerThread, this, i)));
        }
        c = ctl_.load();
        threads_[i]->start();
        everPoolSize_++;
    }
    return 0;
}

int ThreadPoolExecutor::getCorePoolSize() const {
    return corePoolSize_;
}

bool ThreadPoolExecutor::setCorePoolSize(int32_t corePoolSize) {
    if (corePoolSize < 0)
        return false;
    size_t delta = corePoolSize - this->corePoolSize_;
    this->corePoolSize_ = corePoolSize;
    if (workerCountOf(ctl_.load()) > corePoolSize) {
        releaseNonCoreThreads();
        return true;
    }
    else if (delta > 0) {
        // We don't really know how many new threads are "needed".
        // As a heuristic, prestart enough new workers (up to new
        // core size) to handle the current number of tasks in
        // queue, but stop if queue becomes empty while doing so.
        int k = std::min(delta, (size_t)workQueues_->size());
        int32_t c = ctl_.load();
        while (k-- > 0) {
            if (compareAndIncrementWorkerCount(c)) {
                threads_.emplace_back(new Thread(std::bind(&ThreadPoolExecutor::coreWorkerThread, this, corePoolSize_ + k)));
            }
            c = ctl_.load();
            if (workQueues_->empty())
                break;
        }
    }
    return true;
}

void ThreadPoolExecutor::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    advanceRunState(SHUTDOWN);
}

void ThreadPoolExecutor::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        advanceRunState(STOP);
    }
    int32_t c = ctl_.load();
    if (workerCountOf(ctl_.load()) != 0) {
        //释放所有线程资源
        releaseWorkers();
    }

    if (ctl_.compare_exchange_strong(c, ctlOf(TIDYING, 0))) {
        terminated();
        std::lock_guard<std::mutex> lock(mutex_);
        ctl_ = ctlOf(TERMINATED, 0);
    }
}

bool ThreadPoolExecutor::isShutDown() {
    return runStateOf(ctl_.load()) == SHUTDOWN;
}

bool ThreadPoolExecutor::isTerminated() {
    return runStateAtLeast(ctl_.load(), TERMINATED);
}

void ThreadPoolExecutor::advanceRunState(int32_t targetState) {
    for (;;) {
        int32_t c = ctl_.load();
        if (runStateAtLeast(c, targetState) ||
                ctl_.compare_exchange_strong(c, ctlOf(targetState, workerCountOf(c))))
            break;
    }
}

void ThreadPoolExecutor::coreWorkerThread(size_t queueIdex) {
    setCurrentThreadName(prefix_);
    Runnable task;
    std::unique_lock<std::mutex> lk(mutex_);
    while(runStateOf(ctl_.load()) <= SHUTDOWN) {
        if (workQueues_->operator[](queueIdex).is_empty()) {
            notEmpty_.wait(lk);
        }
        if(workQueues_->operator[](queueIdex).try_pop(task)) {
            task();
        }
    }
}

void ThreadPoolExecutor::workerThread(size_t queueIdex) {
    setCurrentThreadName(prefix_);
    Runnable task;
    std::unique_lock<std::mutex> lk(mutex_);
    while(runStateOf(ctl_.load()) <= SHUTDOWN) {
        if (workQueues_->operator[](queueIdex).is_empty()) {
            notEmpty_.wait(lk);
        }
        if(workQueues_->operator[](queueIdex).try_pop(task)) {
            task();
        } else {
            if(!nonCoreThreadAlive_) {
                return;
            }
        }
    }
}
