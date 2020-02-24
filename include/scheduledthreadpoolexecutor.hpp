#ifndef SCHEDULEDTHREADPOOLEXECUTOR_H
#define SCHEDULEDTHREADPOOLEXECUTOR_H

#include <functional>
#include <sstream>
#include <algorithm>

#include "threadpoolexecutor.hpp"
#include "semaphore.hpp"

/**
 * @brief 定时任务封装
 */
struct TimerTask : public Runnable {
    template<typename F>
    /**
     * @brief TimerTask 构造函数
     *
     * @param initDelay 第一次调用的延迟
     * @param interval 每次调用间隔
     * @param fixedRate 是否等间隔运行,true执行间隔确定,false每次调用经过相同延迟
     * @param f lambda或Runnable
     */
    TimerTask(const std::chrono::nanoseconds& initDelay,
              const std::chrono::nanoseconds& interval,
              bool fixedRate, F&& f)
        : Runnable(std::move(f)),
          initialDelay_(initDelay),
          interval_(interval),
          fixedRate_(fixedRate),
          callTime_(std::chrono::steady_clock::now() + initDelay) {}

    /**
     * @brief TimerTask 拷贝构造
     *
     * @param rh 另一个TimerTask
     */
    TimerTask(const TimerTask& rh)
        : initialDelay_(rh.initialDelay_),
          interval_(rh.interval_),
          fixedRate_(rh.fixedRate_),
          callTime_(rh.callTime_) {}

    /**
     * @brief TimerTask 默认构造
     */
    TimerTask() = default;

    /**
     * @brief operator= 赋值运算符
     *
     * @param rh 另一个TimerTask
     *
     * @return TimerTask&
     */
    TimerTask& operator=(const TimerTask& rh) {
        initialDelay_ = rh.initialDelay_;
        interval_ = rh.interval_;
        fixedRate_ = rh.fixedRate_;
        callTime_ = rh.callTime_;
        return *this;
    }
    ///初次延迟
    std::chrono::nanoseconds initialDelay_{0};
    ///固定延迟或间隔
    std::chrono::nanoseconds interval_{0};
    ///是否是固定间隔执行
    bool fixedRate_;
    ///下次执行时间
    std::chrono::steady_clock::time_point callTime_;
};

/**
 * @brief 定时任务调度线程池,最大线程数和核心线程数相等
 */
class ScheduledThreadPoolExecutor: public ThreadPoolExecutor {
    private:
        /**
         * @brief 小顶堆比较操作
         */
        struct Comp {
            public:
                bool operator()(const std::shared_ptr<TimerTask>& t1, const std::shared_ptr<TimerTask>& t2) {
                    return t1->callTime_ > t2->callTime_;
                }
        };

        /**
         * @brief 定时任务队列
         */
        std::vector<std::shared_ptr<TimerTask>> timerTasks_;
        Semaphore sem_{0};

    public:
        /**
         * @brief ScheduledThreadPoolExecutor 构造函数
         *
         * @param corePoolSize 核心线程数量
         * @param prefix 线程名前缀
         */
        ScheduledThreadPoolExecutor(int corePoolSize, const std::string& prefix = "")
            : ThreadPoolExecutor(corePoolSize, corePoolSize, prefix),
              timerTasks_() {}

        /**
         * @brief ~ScheduledThreadPoolExecutor 析构函数
         */
        virtual ~ScheduledThreadPoolExecutor() = default;

    private:
        /**
         * @brief scheduledThread 调度线程
         */
        virtual void coreWorkerThread(size_t) {
            std::chrono::steady_clock::time_point callTime;
            std::shared_ptr<TimerTask> timerTask;
            while(runStateOf(ctl_.load()) <= SHUTDOWN) {
                sem_.wait();
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    timerTask = timerTasks_.front();
                    std::pop_heap(timerTasks_.begin(), timerTasks_.end(), Comp());
                    timerTasks_.pop_back();
                }
                //如果是fixed rate就在执行前更新下次执行时间
                //否则在执行后更新
                callTime = timerTask->callTime_;
                if (timerTask->fixedRate_) {
                    timerTask->callTime_ = std::chrono::steady_clock::now() + timerTask->interval_;
                    std::this_thread::sleep_until(callTime);
                    timerTask->operator()();
                } else {
                    std::this_thread::sleep_until(callTime);
                    timerTask->operator()();
                    timerTask->callTime_ = std::chrono::steady_clock::now() + timerTask->interval_;
                }
                //执行完毕并更新时间完成,将任务放回小顶堆
                {
                    std::lock_guard<std::mutex> lock_guard(mutex_);
                    timerTasks_.push_back(timerTask);
                    std::push_heap(timerTasks_.begin(), timerTasks_.end(), Comp());
                }
                sem_.post();
            }
        }

    public:
        //隐藏父类某些函数
        virtual void execute()                  = delete;
        virtual bool addWorker()                = delete;
        virtual void workerThread()             = delete;
        virtual void setMaxPoolSize()           = delete;
        virtual bool keepNonCoreThreadAlive()   = delete;
        virtual void releaseNonCoreThreads(int) = delete;


    public:
        template<typename F>
        /**
         * @brief schedule 在将来某个时候执行给定的任务,
         *                 任务可以在新线程或现有的合并的线程中执行,
         *                 会抛出异常
         *
         * @param f 要提交的任务(Runnable或函数或lambda,不能是Runnable::sptr)
         * @param delay 固定延迟
         */
        void schedule(F f, const std::chrono::nanoseconds& delay) {
            int32_t c = ctl_.load();
            int32_t rs = runStateOf(c);
            int32_t wc = workerCountOf(c);
            if (rs >= SHUTDOWN)
                reject(Runnable(std::move(f)));
            {
                std::lock_guard<std::mutex> lock(mutex_);
                timerTasks_.emplace_back(new TimerTask(delay, delay, false, std::move(f)));
                std::push_heap(timerTasks_.begin(), timerTasks_.end(), Comp());
            }
            sem_.post();
            if(wc < corePoolSize_ && compareAndIncrementWorkerCount(c)) {
                threads_.emplace_back(new Thread(std::bind(&ScheduledThreadPoolExecutor::coreWorkerThread, this, 0), prefix_));
                threads_.back()->start();
                everPoolSize_++;
            }
        }

        /**
         * @brief schedule 在将来某个时候执行给定的任务,
         *                 任务可以在新线程或现有的合并的线程中执行,
         *                 会抛出异常
         *
         * @param f 要提交的任务(std::shared_ptr<TimerTask>)
         */
        void schedule(const std::shared_ptr<TimerTask>& f) {
            int32_t c = ctl_.load();
            int32_t rs = runStateOf(c);
            int32_t wc = workerCountOf(c);
            if (rs >= SHUTDOWN)
                reject(f);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                timerTasks_.push_back(f);
                std::push_heap(timerTasks_.begin(), timerTasks_.end(), Comp());
            }
            sem_.post();
            if(wc < corePoolSize_ && compareAndIncrementWorkerCount(c)) {
                threads_.emplace_back(new Thread(std::bind(&ScheduledThreadPoolExecutor::coreWorkerThread, this, 9), prefix_));
                threads_.back()->start();
                everPoolSize_++;
            }
        }

        template<typename F>
        /**
         * @brief scheduleAtFixedRate 固定间隔调用
         *
         * @param f 要提交的任务(Runnable或函数或lambda,不能是Runnable::sptr)
         * @param initialDelay 初始延迟
         * @param period 固定间隔
         */
        void scheduleAtFixedRate(F f,
                                 const std::chrono::nanoseconds& initialDelay,
                                 const std::chrono::nanoseconds& period) {
            int32_t c = ctl_.load();
            int32_t rs = runStateOf(c);
            int32_t wc = workerCountOf(c);
            if (rs >= SHUTDOWN)
                reject(Runnable(std::move(f)));
            {
                std::lock_guard<std::mutex> lock(mutex_);
                timerTasks_.emplace_back(new TimerTask(initialDelay, period, true, std::move(f)));
                std::push_heap(timerTasks_.begin(), timerTasks_.end(), Comp());
            }
            sem_.post();
            if(wc < corePoolSize_ && compareAndIncrementWorkerCount(c)) {
                threads_.emplace_back(new Thread(std::bind(&ScheduledThreadPoolExecutor::coreWorkerThread, this, 0), prefix_));
                threads_.back()->start();
                everPoolSize_++;
            }
        }

        template<typename F>
        /**
         * @brief scheduleAtFixedDelay 固定延迟调用
         *
        * @param f 要提交的任务(Runnable或函数或lambda,不能是Runnable::sptr)
         * @param initialDelay 初始延迟
         * @param delay 固定延迟
         */
        void scheduleAtFixedDelay(F f,
                                  const std::chrono::nanoseconds& initialDelay,
                                  const std::chrono::nanoseconds& delay) {
            int32_t c = ctl_.load();
            int32_t rs = runStateOf(c);
            int32_t wc = workerCountOf(c);
            if (rs >= SHUTDOWN)
                reject(Runnable(std::move(f)));
            {
                std::lock_guard<std::mutex> lock(mutex_);
                timerTasks_.emplace_back(initialDelay, delay, false, std::move(f));
                std::push_heap(timerTasks_.begin(), timerTasks_.end(), Comp());
            }
            sem_.post();
            if(wc < corePoolSize_ && compareAndIncrementWorkerCount(c)) {
                threads_.emplace_back(new Thread(std::bind(&ScheduledThreadPoolExecutor::coreWorkerThread, this, 0), prefix_));
                threads_.back()->start();
                everPoolSize_++;
            }
        }

        /**
         * @brief preStartCoreThreads 提前启动核心线程
         *
         * @return
         */
        virtual int preStartCoreThreads() {
            int32_t c = ctl_.load();
            for (int i = 0; i < corePoolSize_; ++i) {
                if (compareAndIncrementWorkerCount(c)) {
                    threads_.emplace_back(new Thread(std::bind(&ScheduledThreadPoolExecutor::coreWorkerThread,
                                                     this, 0), prefix_));
                }
                c = ctl_.load();
                threads_[i]->start();
                everPoolSize_++;
            }
            return everPoolSize_;
        }

        std::string toString() const {
            int32_t c = ctl_.load();
            std::string rs = (runStateLessThan(c, SHUTDOWN) ? "Running" :
                              (runStateAtLeast(c, TERMINATED) ? "Terminated" :
                               "ShuttingDown"));
            std::stringstream ss;
            ss << "STATE="               << rs
               << " EVER_POOL_SIZE="     << everPoolSize_
               << " CORE_POOL_SIZE="     << corePoolSize_
               << " TASK_COUNT="         << getTaskCount();
            return ss.str();
        }

        virtual long getTaskCount() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return timerTasks_.size();
        }

};

#endif /* SCHEDULEDTHREADPOOLEXECUTOR_H */
