#include <sstream>
#include <functional>
#include <stdexcept>

#include "threadpoolexecutor.hpp"

ThreadPoolExecutor::ThreadPoolExecutor(int32_t corePoolSize,
                                       int32_t maxPoolSize,
                                       const std::vector<BlockingQueue<Runnable::sptr>>& workQueue,
                                       const RejectedExecutionHandler& handler,
                                       const std::string& prefix
                                      )
    : corePoolSize_(corePoolSize),
      maxPoolSize_(maxPoolSize),
      prefix_(prefix),
      ctl_(ctlOf(RUNNING, 0)),
      workQueues_(workQueue),
      rejectHandler_(new RejectedExecutionHandler(handler)) {

    if (corePoolSize < 0               ||
            maxPoolSize <= 0           ||
            maxPoolSize < corePoolSize)
        throw std::logic_error("parameter value is wrong");
}

ThreadPoolExecutor::ThreadPoolExecutor(int32_t corePoolSize,
                                       int32_t maxPoolSize,
                                       const std::vector<BlockingQueue<Runnable::sptr>>& workQueue,
                                       RejectedExecutionHandler* handler,
                                       const std::string& prefix
                                      )
    : corePoolSize_(corePoolSize),
      maxPoolSize_(maxPoolSize),
      prefix_(prefix),
      ctl_(ctlOf(RUNNING, 0)),
      workQueues_(workQueue),
      rejectHandler_(handler) {

    if (corePoolSize < 0               ||
            maxPoolSize <= 0           ||
            maxPoolSize < corePoolSize)
        throw std::logic_error("parameter value is wrong");
}

ThreadPoolExecutor::ThreadPoolExecutor(int32_t corePoolSize,
                                       int32_t maxPoolSize,
                                       const std::string& prefix
                                      )
    : corePoolSize_(corePoolSize),
      maxPoolSize_(maxPoolSize),
      prefix_(prefix),
      ctl_(ctlOf(RUNNING, 0)),
      workQueues_(corePoolSize),
      rejectHandler_(new RejectedExecutionHandler()) {

    if (corePoolSize < 0               ||
            maxPoolSize <= 0           ||
            maxPoolSize < corePoolSize)
        throw std::logic_error("parameter value is wrong");
}

ThreadPoolExecutor::~ThreadPoolExecutor() {
    for (auto t : threads_) {
        if (t->joinable()) {
            t->join();
        }
    }
}

bool ThreadPoolExecutor::keepNonCoreThreadAlive() const {
    return keepNonCoreThreadAlive_;
}

void ThreadPoolExecutor::keepNonCoreThreadAlive(bool value) {
    if (value != keepNonCoreThreadAlive_) {
        keepNonCoreThreadAlive_ = value;
    }
    if (!keepNonCoreThreadAlive_) {
        releaseNonCoreThreads();
    }
}

void ThreadPoolExecutor::releaseNonCoreThreads() {
    keepNonCoreThreadAlive_ = false;
    notEmpty_.notify_all();
    for (int i = threads_.size() - 1; i >= corePoolSize_; --i) {
        if (threads_[i] != nullptr && threads_[i]->isIdle() && threads_[i]->joinable()) {
            threads_[i]->join();
            decrementWorkerCount();
            threads_.pop_back();
            workQueues_.pop_back();
        }
    }
    threads_.shrink_to_fit();
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

bool ThreadPoolExecutor::addWorker(Runnable::sptr task, bool core) {
    int32_t c = 0;
    int32_t rs = 0;
    int32_t wc = 0;
    for (;;) {
        c = ctl_.load();
        rs = runStateOf(c);
        if (rs >= SHUTDOWN)
            return false;
        for (;;) {
            wc = workerCountOf(c);
            if (wc >= (core ? corePoolSize_ : maxPoolSize_)) {
                if (core) {
                    workQueues_[++submitId_ % corePoolSize_].put(task);
                    notEmpty_.notify_all();
                    return true;
                } else {
                    workQueues_[(submitId_++ % corePoolSize_) + corePoolSize_].put(task);
                    notEmpty_.notify_all();
                    return true;
                }
            }
            if(compareAndIncrementWorkerCount(c)) {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    workQueues_.push_back(BlockingQueue<Runnable::sptr>());
                    workQueues_.back().put(task);
                }
                wc = workerCountOf(c);
                if (core || wc <= corePoolSize_) {
                    threads_.emplace_back(new Thread(std::bind(&ThreadPoolExecutor::coreWorkerThread,
                                                     this, workQueues_.size() - 1), prefix_));
                } else {
                    threads_.emplace_back(new Thread(std::bind(&ThreadPoolExecutor::workerThread,
                                                     this, workQueues_.size() - 1), prefix_));
                }
                threads_.back()->start();
                everPoolSize_++;
                return true;
            }
            c = ctl_.load();  // Re-read ctl
            if(runStateOf(c) != rs)
                break;
        }
    }
}

bool ThreadPoolExecutor::addWorker(Runnable task, bool core) {
    int32_t c = 0;
    int32_t rs = 0;
    int32_t wc = 0;
    for (;;) {
        c = ctl_.load();
        rs = runStateOf(c);
        if (rs >= SHUTDOWN)
            return false;
        for (;;) {
            wc = workerCountOf(c);
            if (wc >= (core ? corePoolSize_ : maxPoolSize_)) {
                if (core) {
                    workQueues_[++submitId_ % corePoolSize_].put(std::make_shared<Runnable>(std::move(task)));
                    notEmpty_.notify_all();
                    return true;
                } else {
                    workQueues_[(submitId_++ % corePoolSize_) + corePoolSize_].put(std::make_shared<Runnable>(std::move(task)));
                    notEmpty_.notify_all();
                    return true;
                }
            }
            if(compareAndIncrementWorkerCount(c)) {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    workQueues_.push_back(BlockingQueue<Runnable::sptr>());
                    workQueues_.back().put(std::make_shared<Runnable>(std::move(task)));
                }
                if (core) {
                    threads_.emplace_back(new Thread(std::bind(&ThreadPoolExecutor::coreWorkerThread,
                                                     this, workQueues_.size() - 1), prefix_));
                } else {
                    threads_.emplace_back(new Thread(std::bind(&ThreadPoolExecutor::workerThread,
                                                     this, workQueues_.size() - 1), prefix_));
                }
                threads_.back()->start();
                everPoolSize_++;
                return true;
            }
            c = ctl_.load();  // Re-read ctl
            if(runStateOf(c) != rs)
                break;
        }
    }
}

bool ThreadPoolExecutor::execute(Runnable::sptr command, bool core) {
    int32_t c = ctl_.load();
    if(addWorker(command, core)) {
        return true;
    }
    c = ctl_.load();
    if (!isRunning(c)) {
        reject(command);
        return false;
    }
    return false;
}

bool ThreadPoolExecutor::execute(Runnable& command, bool core) {
    int32_t c = ctl_.load();
    if(isRunning(c)) {
        if(addWorker(std::make_shared<Runnable>(command), core)) {
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

bool ThreadPoolExecutor::execute(BlockingQueue<Runnable::sptr>& commands, bool core) {
    int32_t c = ctl_.load();
    if (core) {
        for (int i = 0; i < commands.size(); ++i) {
            c = ctl_.load();
            auto command = commands.take();
            if(addWorker(command, core)) {
                return true;
            }
            c = ctl_.load();
            if (!isRunning(c)) {
                reject(command);
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
            workQueues_.emplace_back(commands);
            threads_.emplace_back(new Thread(std::bind(&ThreadPoolExecutor::workerThread,
                                             this, workQueues_.size() - 1), prefix_));
            threads_.back()->start();
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
       << " TASK_COUNT="         << getTaskCount();
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
    for (auto e : workQueues_) {
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

int ThreadPoolExecutor::preStartCoreThreads() {
    int32_t c = ctl_.load();
    for (int i = 0; i < corePoolSize_; ++i) {
        if (compareAndIncrementWorkerCount(c)) {
            threads_.emplace_back(new Thread(std::bind(&ThreadPoolExecutor::coreWorkerThread, this, i), prefix_));
        }
        c = ctl_.load();
        threads_[i]->start();
        everPoolSize_++;
    }
    return everPoolSize_;
}

int ThreadPoolExecutor::getCorePoolSize() const {
    return corePoolSize_;
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
    Runnable::sptr task;
    std::unique_lock<std::mutex> lk(mutex_);
    while(runStateOf(ctl_.load()) <= SHUTDOWN) {
        if (workQueues_[queueIdex].is_empty()) {
            notEmpty_.wait(lk);
        }
        if(workQueues_[queueIdex].try_pop(task)) {
            task->operator()();
        }
    }
}

void ThreadPoolExecutor::workerThread(size_t queueIdex) {
    Runnable::sptr task;
    std::unique_lock<std::mutex> lk(mutex_);
    while(runStateOf(ctl_.load()) <= SHUTDOWN) {
        if(workQueues_[queueIdex].is_empty()) {
            notEmpty_.wait(lk);
        }
        if(workQueues_[queueIdex].try_pop(task)) {
            task->operator()();
        }
        if(!keepNonCoreThreadAlive_) {
            return;
        }
    }
}
