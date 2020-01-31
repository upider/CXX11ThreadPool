#include <sstream>
#include <stdexcept>
#include <pthread.h>
#include <signal.h>

#include "threadpool.hpp"

ThreadPoolExecutor::ThreadPoolExecutor(int32_t corePoolSize,
                                       int32_t maxPoolSize,
                                       BlockingQueue<Runnable> workQueue,
                                       RejectedExecutionHandler handler)
    : _corePoolSize(corePoolSize),
      _maxPoolSize(maxPoolSize),
      _ctl(ctlOf(RUNNING, 0)) {

    if (corePoolSize < 0               ||
            maxPoolSize <= 0           ||
            maxPoolSize < corePoolSize)
        throw std::logic_error("parameter value is wrong");

    this->_workQueue.reset(new BlockingQueue<Runnable>(workQueue));
    this->_handler.reset(new RejectedExecutionHandler(handler));
    start();
}

ThreadPoolExecutor::ThreadPoolExecutor(int32_t corePoolSize,
                                       int32_t maxPoolSize,
                                       BlockingQueue<Runnable>* workQueue,
                                       RejectedExecutionHandler* handler)
    : _corePoolSize(corePoolSize),
      _maxPoolSize(maxPoolSize),
      _ctl(ctlOf(RUNNING, 0)) {

    if (corePoolSize < 0               ||
            maxPoolSize <= 0           ||
            maxPoolSize < corePoolSize)
        throw std::logic_error("parameter value is wrong");

    this->_workQueue.reset(workQueue);
    this->_handler.reset(handler);
    start();
}

ThreadPoolExecutor::ThreadPoolExecutor(int32_t corePoolSize,
                                       int32_t maxPoolSize)
    : _corePoolSize(corePoolSize),
      _maxPoolSize(maxPoolSize),
      _ctl(ctlOf(RUNNING, 0)) {

    if (corePoolSize < 0               ||
            maxPoolSize <= 0           ||
            maxPoolSize < corePoolSize)
        throw std::logic_error("parameter value is wrong");

    this->_workQueue.reset(new BlockingQueue<Runnable>);
    this->_handler.reset(new RejectedExecutionHandler);
    start();
}

ThreadPoolExecutor::~ThreadPoolExecutor() {
    advanceRunState(TERMINATED);
    for (std::thread& thread : _threads) {
        if(thread.joinable())
            thread.join(); // 等待任务结束， 前提：线程一定会执行完
    }
}

bool ThreadPoolExecutor::allowsCoreThreadTimeOut() const {
    return _allowCoreThreadTimeOut;
}

void ThreadPoolExecutor::allowCoreThreadTimeOut(bool value) {
    if (value != _allowCoreThreadTimeOut) {
        _allowCoreThreadTimeOut = value;
    }
    if (value) {
        releaseIdleWorkers();
    }
}

void ThreadPoolExecutor::releaseIdleWorkers(bool onlyOne) {
    std::stringstream ss;
    std::lock_guard<std::mutex> lock(_mutex);
    for (int i = _corePoolSize; i < _maxPoolSize; ++i) {
        if (_threads[i].joinable()) {
            _threads[i].join();
        }
        if (onlyOne)
            break;
    }
}

void ThreadPoolExecutor::releaseWorkers() {
    std::lock_guard<std::mutex> lock(_mutex);
    std::stringstream ss;
    for (int i = 0; i < _maxPoolSize; ++i) {
        if (i <= _corePoolSize) {
            if (_threads[i].joinable()) {
                _threads[i].join();
            }
        } else {
            if (_threads[i].joinable()) {
                _threads[i].join();
            }
        }
    }
}

bool ThreadPoolExecutor::addWorker(const Runnable firstTask, bool core) {
    for (;;) {
        int c = _ctl.load();
        int rs = runStateOf(c);

        // Check if queue empty only if necessary.
        if (rs >= SHUTDOWN && !(rs == SHUTDOWN && !_workQueue->is_empty()))
            return false;

        for (;;) {
            int wc = workerCountOf(c);
            if (wc >= CAPACITY || wc >= (core ? _corePoolSize : _maxPoolSize))
                goto goon;
            if(compareAndIncrementWorkerCount(c)) {
                std::lock_guard<std::mutex> lock(_mutex);
                _threads.push_back(std::thread(&ThreadPoolExecutor::workerThreadAdded, this));
                goto goon;
            }
            c = _ctl.load();  // Re-read ctl
            if(runStateOf(c) != rs)
                break;
            // else CAS failed due to workerCount change; retry inner loop
        }
    }

goon:
    bool workerAdded = false;
    std::lock_guard<std::mutex> lock(_mutex);
    // Recheck while holding lock.
    // Back out on ThreadFactory failure or if
    // shut down before lock acquired.
    int rs = runStateOf(_ctl.load());

    if (rs < SHUTDOWN) {
        _workQueue->put(std::move(firstTask));
        workerAdded = true;
    }
    return workerAdded;
}

bool ThreadPoolExecutor::execute(const Runnable command) {
    /*
     * Proceed in 3 steps:
     *
     * 1. If fewer than corePoolSize threads are running, try to
     * start a new thread with the given command as its first
     * task.  The call to addWorker atomically checks runState and
     * workerCount, and so prevents false alarms that would add
     * threads when it shouldn't, by returning false.
     *
     * 2. If a task can be successfully queued, then we still need
     * to double-check whether we should have added a thread
     * (because existing ones died since last checking) or that
     * the pool shut down since entry into this method. So we
     * recheck state and if necessary roll back the enqueuing if
     * stopped, or start a new thread if there are none.
     *
     * 3. If we cannot queue task, then we try to add a new
     * thread.  If it fails, we know we are shut down or saturated
     * and so reject the task.
     */
    int c = _ctl.load();
    if(workerCountOf(c) < _corePoolSize) {
        if (addWorker(command, true))
            return true;
    }
    c = _ctl.load();
    if(isRunning(c)) {
        if(addWorker(command, false)) {
            return true;
        }
        c = _ctl.load();
        if (!isRunning(c)) {
            reject(command);
            return false;
        }
    }
    return false;
}

void ThreadPoolExecutor::setRejectedExecutionHandler(RejectedExecutionHandler handler) {
    _handler.reset(new RejectedExecutionHandler(handler));
}

std::string ThreadPoolExecutor::toString() const {
    int c = _ctl.load();
    std::string rs = (runStateLessThan(c, SHUTDOWN) ? "Running" :
                      (runStateAtLeast(c, TERMINATED) ? "Terminated" :
                       "Shutting down"));
    std::stringstream ss;
    ss << "STATE="           << rs
       << " LARGEST_POOL_SIZE="      << _threads.size()
       << " CORE_POOL_SIZE=" << _corePoolSize
       << " MAX_POOL_SIZE="  << _maxPoolSize
       << " TASK_SIZE="      << _workQueue->size();
    return ss.str();
}

int ThreadPoolExecutor::getActiveCount() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _threadNum;
}

long ThreadPoolExecutor::getTaskCount() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _workQueue->size();
}

std::string ThreadPoolExecutor::getKeepAliveTime() const {
    std::stringstream ss;

    return ss.str();
}

void ThreadPoolExecutor::setMaxPoolSize(int32_t maxPoolSize) {
    if (maxPoolSize <= 0 || maxPoolSize < _corePoolSize)
        return;
    this->_maxPoolSize = maxPoolSize;
    if (workerCountOf(_ctl.load()) > _maxPoolSize) {
        releaseIdleWorkers();
    }
}

int ThreadPoolExecutor::getLargestPoolSize() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _threads.size();
}

int ThreadPoolExecutor::prestartAllCoreThreads() {
    for(auto& t : _threads) {
        if(t.joinable())
            t.join();
    }
    return 0;
}

int ThreadPoolExecutor::getCorePoolSize() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _corePoolSize;
}

bool ThreadPoolExecutor::setCorePoolSize(int32_t corePoolSize) {
    if (corePoolSize < 0)
        return false;
    size_t delta = corePoolSize - this->_corePoolSize;
    this->_corePoolSize = corePoolSize;
    if (workerCountOf(_ctl.load()) > corePoolSize) {
        releaseIdleWorkers();
        return true;
    }
    else if (delta > 0) {
        // We don't really know how many new threads are "needed".
        // As a heuristic, prestart enough new workers (up to new
        // core size) to handle the current number of tasks in
        // queue, but stop if queue becomes empty while doing so.
        int k = std::min(delta, (size_t)_workQueue->size());
        while (k-- > 0) {
            int c = _ctl.load();
            _threads.push_back(
                std::thread(&ThreadPoolExecutor::workerThread, this));
            compareAndIncrementWorkerCount(c);
            if (_workQueue->is_empty())
                break;
        }
    }
    return true;
}

void ThreadPoolExecutor::shutdown() {
    std::lock_guard<std::mutex> lock(_mutex);
    advanceRunState(SHUTDOWN);
}

void ThreadPoolExecutor::stop() {
    {
        std::lock_guard<std::mutex> lock(_mutex);
        advanceRunState(STOP);
    }
    releaseWorkers();
    tryTerminate();
}

void ThreadPoolExecutor::tryTerminate() {
    for (;;) {
        int c = _ctl.load();
        if (isRunning(c) || runStateAtLeast(c, TIDYING) ||
                (runStateOf(c) == SHUTDOWN && ! _workQueue->is_empty()))
            return;
        if (workerCountOf(c) != 0) { // Eligible to terminate
            releaseIdleWorkers(true);
            return;
        }

        std::lock_guard<std::mutex> lock(_mutex);
        if (_ctl.compare_exchange_strong(c, ctlOf(TIDYING, 0))) {
            terminated();
            _ctl = ctlOf(TERMINATED, 0);
            return;
        }
    }
}

bool ThreadPoolExecutor::isShutDown() {
    return ! isRunning(_ctl.load());
}

bool ThreadPoolExecutor::isTerminated() {
    return runStateAtLeast(_ctl.load(), TERMINATED);
}

void ThreadPoolExecutor::advanceRunState(int32_t targetState) {
    for (;;) {
        int c = _ctl.load();
        if (runStateAtLeast(c, targetState) ||
                _ctl.compare_exchange_strong(c, ctlOf(targetState, workerCountOf(c))))
            break;
    }
}

void ThreadPoolExecutor::start() {
    try {
        int c = _ctl.load();
        for(int32_t i = 0; i < _corePoolSize; ++i) {
            _threads.push_back(
                std::thread(&ThreadPoolExecutor::workerThread, this));
            compareAndIncrementWorkerCount(c);
        }
    }
    catch(...)
    {
        throw;
    }
}

void ThreadPoolExecutor::workerThread() {
    while(runStateOf(_ctl.load()) == RUNNING) {
        Runnable task;
        if(_workQueue->try_pop(task)) {
            _threadNum++;
            task();
            _threadNum--;
        }
        else {
            std::this_thread::yield();
        }
    }
}

void ThreadPoolExecutor::workerThreadAdded() {
    while(runStateOf(_ctl.load()) == RUNNING) {
        Runnable task;
        if(_workQueue->try_pop(task)) {
            _threadNum++;
            task();
            _threadNum--;
        }
        else {
            int min = _allowCoreThreadTimeOut ? 0 : _corePoolSize;
            if (min == 0) {
                std::this_thread::yield();
            }
            else {
                return;
            }
        }
    }
}
