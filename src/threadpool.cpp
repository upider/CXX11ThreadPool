#include <sstream>
#include <stdexcept>

#include "threadpool.hpp"

ThreadPoolExecutor::ThreadPoolExecutor(int32_t corePoolSize,
                                       int32_t maxPoolSize,
                                       long keepAliveTime,
                                       TimeUnit unit,
                                       BlockingQueue<Runnable> workQueue,
                                       RejectedExecutionHandler handler)
    : _corePoolSize(corePoolSize),
      _maxPoolSize(maxPoolSize),
      _keepAliveTime(keepAliveTime),
      _unit(unit),
      _ctl(ctlOf(RUNNING, 0)) {

    if (corePoolSize < 0               ||
            maxPoolSize <= 0           ||
            maxPoolSize < corePoolSize ||
            keepAliveTime < 0)
        throw std::logic_error("parameter value is wrong");

    this->_workQueue.reset(new BlockingQueue<Runnable>(workQueue));
    this->_handler.reset(new RejectedExecutionHandler(handler));
    start();
}

ThreadPoolExecutor::ThreadPoolExecutor(int32_t corePoolSize,
                                       int32_t maxPoolSize,
                                       long keepAliveTime,
                                       TimeUnit unit,
                                       BlockingQueue<Runnable>* workQueue)
    : _corePoolSize(corePoolSize),
      _maxPoolSize(maxPoolSize),
      _keepAliveTime(keepAliveTime),
      _unit(unit),
      _ctl(ctlOf(RUNNING, 0)) {

    if (corePoolSize < 0               ||
            maxPoolSize <= 0           ||
            maxPoolSize < corePoolSize ||
            keepAliveTime < 0)
        throw std::logic_error("parameter value is wrong");

    this->_workQueue.reset(workQueue);
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
    if (value && _keepAliveTime <= 0)
        return;
    if (value != _allowCoreThreadTimeOut) {
        _allowCoreThreadTimeOut = value;
        if (value)
            interruptIdleWorkers();
    }
}

void ThreadPoolExecutor::interruptIdleWorkers() {}

void ThreadPoolExecutor::interruptIdleWorkers(bool onlyOne) {}

bool ThreadPoolExecutor::addWorker(const Runnable firstTask, bool core) {
    for (;;) {
        int c = _ctl.load();
        int rs = runStateOf(c);

        // Check if queue empty only if necessary.
        if (rs >= SHUTDOWN && !(rs == SHUTDOWN && !_workQueue->isEmpty()))
            return false;

        for (;;) {
            int wc = workerCountOf(c);
            if (wc >= CAPACITY || wc >= (core ? _corePoolSize : _maxPoolSize))
                goto goon;
            if(compareAndIncrementWorkerCount(c)) {
                std::lock_guard<std::mutex> lock(_mutex);
                _threads.push_back(std::thread(&ThreadPoolExecutor::workerThread, this));
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
        int s = _workQueue->size();
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

std::string ThreadPoolExecutor::toString() const {
    std::stringstream ss;
    ss << "pool_size=" << getPoolSize()
       << " core_pool_size=" << _corePoolSize
       << " max_pool_size=" << _maxPoolSize
       << " task_size=" << _workQueue->size();
    return ss.str();
}

int ThreadPoolExecutor::getActiveCount() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _threadNum;
}

void ThreadPoolExecutor::setKeepAliveTime(long time, TimeUnit unit) {
    if (time < 0)
        return;
    if (time == 0 && allowsCoreThreadTimeOut())
        return;
    long delta = time - this->_keepAliveTime;
    this->_keepAliveTime = time;
    if (delta < 0)
        interruptIdleWorkers();
}

void ThreadPoolExecutor::setMaxPoolSize(int32_t maxPoolSize) {
    if (maxPoolSize <= 0 || maxPoolSize < _corePoolSize)
        return;
    this->_maxPoolSize = maxPoolSize;
    if (workerCountOf(_ctl.load()) > _maxPoolSize)
        interruptIdleWorkers();
}

int ThreadPoolExecutor::getPoolSize() const {
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

void ThreadPoolExecutor::setCorePoolSize(int32_t corePoolSize) {}

void ThreadPoolExecutor::shutdown() {
    std::lock_guard<std::mutex> lock(_mutex);
    advanceRunState(SHUTDOWN);
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
        std::lock_guard<std::mutex> lock(_mutex);
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
