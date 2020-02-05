#ifndef THREAD_HPP
#define THREAD_HPP

#include <string>
#include <cstring>
#include <thread>
#include <memory>
#include <chrono>
#include <memory>
#include <atomic>
#include <queue>

#include <sys/syscall.h>
#include <sys/signal.h>
#include <unistd.h>

#include "runnable.hpp"

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

static std::string getThreadName(std::thread::id id) {
    std::array<char, 16> buf;
    if (id != std::thread::id() &&
            pthread_getname_np(stdTidToPthreadId(id), buf.data(), buf.size()) == 0) {
        return std::string(buf.data());
    } else {
        return "";
    }
}

static std::string getCurrentThreadName() {
    return getThreadName(std::this_thread::get_id());
}

static bool setThreadName(std::thread::id tid, const std::string& name) {
    auto str = name.substr(0, 15);
    char buf[16] = {};
    std::memcpy(buf, str.data(), str.size());
    auto id = stdTidToPthreadId(tid);
    return 0 == pthread_setname_np(id, buf);
}

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

static bool setCurrentThreadName(const std::string& name) {
    return setThreadName(std::this_thread::get_id(), name);
}

class ThreadPoolExecutor;
class Thread {
    protected:
        bool                                   idle_{true};
        int                                    prio_;
        pid_t                                  currentPid_{-1};      ///-1表明线程已经执行完任务,unix底层的线程已经不存在了
        uint64_t                               uniqueId_{nextId_++}; ///当前线程id与系统无关
        std::string                            name_;
        std::thread                            thread_;
        std::atomic<bool>                      yield_{false};
        static std::atomic<uint64_t>           nextId_;
        std::chrono::steady_clock::time_point  lastActiveTime_{std::chrono::steady_clock::now()};

    protected:
        struct Func_base {
            Func_base() = default;
            virtual void call() = 0;
            virtual ~Func_base() {}
        };

        template<typename FunctionType>
        struct Func_t: Func_base {
            Func_t(FunctionType&& f): _f(std::move(f)) {}
            void call() override {
                _f();
            }
            FunctionType _f;
        };

        std::unique_ptr<Func_base> func_uptr_;

    public:
        using sptr = std::shared_ptr<Thread>;

        Thread(int pro, ThreadPoolExecutor* tpe = nullptr): prio_(pro) {}

        Thread(const std::string& name = "", int pro = 20, ThreadPoolExecutor* tpe = nullptr): prio_(pro), name_(name) {}

        template<typename FunctionType>
        /**
         * @brief Thread
         *
         * @param f
         * @param name
         */
        Thread(FunctionType f, const std::string& name = "", int pro = 20, ThreadPoolExecutor* tpe = nullptr)
            : prio_(pro), name_(name), func_uptr_(new Func_t<FunctionType>(std::move(f))) {}

        virtual ~Thread() = default;

    private:
        /**
                 * @brief run 重载实现操作
                 */
        virtual void run() {}

        virtual void executeRun() final {
            setCurrentThreadName(name_);
            idle_ = false;
            lastActiveTime_ = std::chrono::steady_clock::now();
            currentPid_ = syscall(__NR_gettid);
            pthread_setschedprio(pthread_self(), prio_);
            try {
                run();
            } catch(...) {
                idle_ = true;
                throw;
            }
            idle_ = true;
        }

        virtual void executeFunc() final {
            setCurrentThreadName(name_);
            idle_ = false;
            lastActiveTime_ = std::chrono::steady_clock::now();
            currentPid_ = syscall(__NR_gettid);
            pthread_setschedprio(pthread_self(), prio_);
            try {
                func_uptr_->call();
            } catch(...) {
                idle_ = true;
                throw;
            }
            idle_ = true;
        }

    public:
        /**
         * @brief start 开始执行线程,如果构造函数传入了Func,
         *              那么重写的run方法不会执行
         */
        virtual void start() final {
            thread_ = std::thread([this]() {
                if (yield_) {
                    std::this_thread::yield();
                    yield_ = false;
                }

                if (func_uptr_ != nullptr) {
                    executeFunc();
                    func_uptr_.release();
                } else {
                    executeRun();
                }

                currentPid_ = -1;
            });
        }

        /**
         * @brief join 释放线程资源
         */
        virtual void join() final {
            thread_.join();
        }

        /**
         * @brief detach 释放线程
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
         * @return bool
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

        static int getNextId() {
            return nextId_;
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
        virtual pid_t getCurrentPid() const final {
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
         * @brief uniqueId 获取线程uniqueId
         *
         * @return uint64_t 线程uniqueId,便于表示,与操作系统无关
         */
        virtual uint64_t uniqueId() const final {
            return uniqueId_;
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

std::atomic<uint64_t> Thread::nextId_(0);

#endif /* THREAD_HPP */
