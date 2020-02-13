#ifndef THREAD_HPP
#define THREAD_HPP

#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <string>
#include <thread>

#include <sys/syscall.h>
#include <sys/signal.h>
#include <unistd.h>

/**
 * @brief stdTidToPthreadId std::thread::id转换为pthread_t
 *
 * @param tid std::thread::id
 *
 * @return pthread_t
 */
static pthread_t stdTidToPthreadId(std::thread::id tid) {
    static_assert(
        std::is_same<pthread_t, std::thread::native_handle_type>::value,
        "This assumes that the native handle type is pthread_t");
    static_assert(
        sizeof(std::thread::native_handle_type) == sizeof(std::thread::id),
        "This assumes std::thread::id is a thin wrapper around "
        "std::thread::native_handle_type, but that doesn't appear to be true.");
    // In most implementations, std::thread::id is a thin wrapper around
    // std::thread::native_handle_type, which means we can do unsafe things to
    // extract it.
    pthread_t id;
    std::memcpy(&id, &tid, sizeof(id));
    return id;
}

/**
 * @brief getThreadName 得到线程名称
 *
 * @param id std::thread::id
 *
 * @return 当前线程名称
 */
static std::string getThreadName(std::thread::id id) {
    std::array<char, 16> buf;
    if (id != std::thread::id() &&
            pthread_getname_np(stdTidToPthreadId(id), buf.data(), buf.size()) == 0) {
        return std::string(buf.data());
    } else {
        return "";
    }
}

/**
 * @brief getCurrentThreadName 得到当前线程名称
 *
 * @return std::string ThreadName
 */
static std::string getCurrentThreadName() {
    return getThreadName(std::this_thread::get_id());
}

/**
 * @brief setThreadName 设置线程名称
 *
 * @param tid std::thread::id
 * @param name 线程名称
 *
 * @return true - 成功
 */
static bool setThreadName(std::thread::id tid, const std::string& name) {
    auto str = name.substr(0, 15);
    char buf[16] = {};
    std::memcpy(buf, str.data(), str.size());
    auto id = stdTidToPthreadId(tid);
    return 0 == pthread_setname_np(id, buf);
}

/**
 * @brief setThreadName 设置线程名称
 *
 * @param tid std::thread::id
 * @param name 线程名称
 *
 * @return true - 成功
 */
static bool setThreadName(pthread_t pid, const std::string& name) {
    static_assert(
        std::is_same<pthread_t, std::thread::native_handle_type>::value,
        "This assumes that the native handle type is pthread_t");
    static_assert(
        sizeof(std::thread::native_handle_type) == sizeof(std::thread::id),
        "This assumes std::thread::id is a thin wrapper around "
        "std::thread::native_handle_type, but that doesn't appear to be true.");
    // In most implementations, std::thread::id is a thin wrapper around
    // std::thread::native_handle_type, which means we can do unsafe things to
    // extract it.
    std::thread::id id;
    std::memcpy(static_cast<void*>(&id), &pid, sizeof(id));
    return setThreadName(id, name);
}

/**
 * @brief setCurrentThreadName 设置当前线程名称
 *
 * @param name 线程名称
 *
 * @return true - 成功
 */
static bool setCurrentThreadName(const std::string& name) {
    return setThreadName(std::this_thread::get_id(), name);
}

class ThreadPoolExecutor;

/**
 * @brief C++11线程封装类,包含更丰富功能
 */
class Thread {
    protected:
        ///线程优先级
        int                                    prio_;
        ///-1表明线程已经执行完任务,unix底层的线程已经不存在了
        pid_t                                  currentPid_{-1};
        ///线程名前缀
        std::string                            name_;
        ///线程
        std::thread                            thread_;
        ///线程停止标志
        std::atomic_bool                       stop_{true};
        ///线程空闲标志
        std::atomic_bool                       idle_{true};
        ///让出时间片标志
        std::atomic_bool                       yield_{false};
        ///上次活跃时间
        std::chrono::steady_clock::time_point  lastActiveTime_{std::chrono::steady_clock::now()};

    protected:
        /**
         * @brief 包装传进来的lambda函数的虚基类
         */
        struct Func_base {
            Func_base() = default;
            virtual void call() = 0;
            virtual ~Func_base() {}
        };

        template<typename FunctionType>
        /**
         * @brief 包装传进来的lambda函数
         */
        struct Func_t: Func_base {
            Func_t(FunctionType&& f): _f(std::move(f)) {}
            void call() override {
                _f();
            }
            FunctionType _f;
        };

        /**
         * @brief 函数封装
         */
        std::unique_ptr<Func_base> func_uptr_;

    public:
        /**
         * @brief std::shared_ptr<Thread>别名
         */
        using sptr = std::shared_ptr<Thread>;

        /**
         * @brief Thread 构造函数
         *
         * @param pro 优先级
         */
        Thread(int pro): prio_(pro) {}

        /**
         * @brief Thread 构造函数
         *
         * @param name 线程名
         * @param pro 优先级
         */
        Thread(const std::string& name = "", int pro = 20)
            : prio_(pro), name_(name) {}

        template<typename FunctionType>
        /**
         * @brief Thread 构造函数
         *
         * @param f 要执行的任务
         * @param name 线程名
         * @param pro 优先级
         */
        Thread(FunctionType f, const std::string& name = "", int pro = 20)
            : prio_(pro), name_(name), func_uptr_(new Func_t<FunctionType>(std::move(f))) {}

        /**
         * @brief ~Thread 析构函数
         */
        virtual ~Thread() = default;

    protected:
        /**
         * @brief run 重载实现操作
         */
        virtual void run() {}

    private:
        /**
         * @brief executeRun 执行重载的run()函数封装
         */
        virtual void executeRun() final {
            setCurrentThreadName(name_ + std::to_string(syscall(__NR_gettid)));
            currentPid_ = syscall(__NR_gettid);
            idle_.store(false, std::memory_order_relaxed);
            lastActiveTime_ = std::chrono::steady_clock::now();
            pthread_setschedprio(pthread_self(), prio_);
            try {
                run();
            } catch(...) {
                idle_.store(true, std::memory_order_relaxed);
                throw;
            }
            idle_.store(true, std::memory_order_relaxed);
        }

        /**
         * @brief executeFunc 执行函数包装器封装
         */
        virtual void executeFunc() final {
            setCurrentThreadName(name_ + std::to_string(syscall(__NR_gettid)));
            currentPid_ = syscall(__NR_gettid);
            idle_.store(false, std::memory_order_relaxed);
            lastActiveTime_ = std::chrono::steady_clock::now();
            pthread_setschedprio(pthread_self(), prio_);
            try {
                if (func_uptr_ == nullptr) {
                }
                func_uptr_->call();
            } catch(...) {
                idle_.store(true, std::memory_order_relaxed);
                throw;
            }
            idle_.store(true, std::memory_order_relaxed);
        }

    public:
        /**
         * @brief start 开始执行线程,如果构造函数传入了Func,
         *              那么重写的run方法不会执行
         */
        virtual void start() final {
            if (!stop_.load(std::memory_order_relaxed))
                throw std::logic_error("thread already started");
            stop_.store(false, std::memory_order_relaxed);
            thread_ = std::thread([this]() {
                if (yield_) {
                    std::this_thread::yield();
                    yield_ = false;
                }
                if (func_uptr_ != nullptr) {
                    executeFunc();
                } else {
                    executeRun();
                }
                currentPid_ = -1;
            });
        }

        /**
         * @brief join 释放线程资源,如果线程此时是空闲的,那么线程会退出
         */
        virtual void join() final {
            thread_.join();
        }

        /**
         * @brief detach 释放线程,如果线程此时是空闲的,那么线程会退出
         */
        virtual void detach() final {
            thread_.detach();
        }

        /**
         * @brief yield 让出时间片
         */
        virtual void yield() final {
            yield_ = true;
        }

        /**
         * @brief joinable
         *
         * @return bool true-可以执行join或detach
         */
        virtual bool joinable() const final {
            return thread_.joinable();
        }

        /**
         * @brief self 返回线程底层句柄
         *
         * @return handler std::thread::native_handle_type类型的线程句柄
         */
        virtual std::thread::native_handle_type self() final {
            return thread_.native_handle();
        }

        /**
         * @brief getName 得到线程名称
         *
         * @return std::string name线程名称
         */
        virtual std::string getName() const final {
            return name_;
        }

        /**
         * @brief setName 设置线程名称
         *
         * @param name 线程名称
         */
        virtual void setName(const std::string & name)final {
            this->name_ = name;
        }

        /**
         * @brief getCurrentPid 获取Linux底层线程id,
         *                      start之后才有效,-1表示线程已经结束
         *
         * @return pid_t Linux底层线程id
         */
        virtual pid_t getPid() const final {
            return currentPid_;
        }

        /**
         * @brief getLastActiveTime 上一次活跃时间
         *
         * @return std::chrono::steady_clock::time_point 上一次活跃时间
         */
        virtual std::chrono::steady_clock::time_point
        getLastActiveTime()const final {
            return lastActiveTime_;
        }

        /**
         * @brief stdId 获取线程thread::id
         *
         * @return std::thread::id 线程thread::id
         */
        virtual std::thread::id stdId() final {
            return thread_.get_id();
        }

        /**
         * @brief isIdle 查看线程是否空闲,使用的是类内部成员变量,与isRunning不同
         *
         * @return bool true-线程空闲
         */
        virtual bool isIdle() const final {
            return idle_;
        }

        /**
         * @brief isRunning 查看线程是否还在运行,使用pthread_kill向线程发信号
         *
         * @return bool true-还在运行
         */
        virtual bool isRunning() final {
            return !pthread_kill(thread_.native_handle(), 0);
        }

        /**
         * @brief setPrio 设置优先级
         *
         * @param prio 要设置的优先级
         */
        virtual void setPrio(int prio)final {
            prio_ = prio;
        }

        /**
         * @brief getPrio 获取线程优先级
         *
         * @return prio 优先级
         */
        virtual int getPrio()final {
            return prio_;
        }

    public:
        Thread(const Thread&&) = delete;
        Thread& operator=(const Thread&&) = delete;
        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;
};

#endif /* THREAD_HPP */
