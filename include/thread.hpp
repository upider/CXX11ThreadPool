#ifndef THREAD_H
#define THREAD_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <future>
#include <stdexcept>
#include <thread>

void interruption_point();

class interrupt_flag {
    private:
        std::atomic<bool> flag;
        std::condition_variable* thread_cond;
        std::condition_variable_any* thread_cond_any;
        std::mutex set_clear_mutex;

    public:
        interrupt_flag():
            thread_cond(0), thread_cond_any(0) {}

        void set() {
            flag.store(true, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lk(set_clear_mutex);
            if(thread_cond) {
                thread_cond->notify_all();
            }
            else if(thread_cond_any) {
                thread_cond_any->notify_all();
            }
        }

        bool is_set() const {
            return flag.load(std::memory_order_relaxed);
        }

        void set_condition_variable(std::condition_variable& cv) {
            std::lock_guard<std::mutex> lk(set_clear_mutex);
            thread_cond = &cv;
        }

        void clear_condition_variable() {
            std::lock_guard<std::mutex> lk(set_clear_mutex);
            thread_cond = 0;
        }

        template<typename Lockable>
        void wait(std::condition_variable_any& cv, Lockable& lk) {
            struct custom_lock
            {
                interrupt_flag* self;
                Lockable& lk;
                custom_lock(interrupt_flag* self_,
                            std::condition_variable_any& cond,
                            Lockable& lk_):
                    self(self_), lk(lk_) {
                    self->set_clear_mutex.lock();
                    self->thread_cond_any = &cond;
                }
                void unlock() {
                    lk.unlock();
                    self->set_clear_mutex.unlock();
                }
                void lock() {
                    std::lock(self->set_clear_mutex, lk);
                }
                ~custom_lock() {
                    self->thread_cond_any = 0;
                    self->set_clear_mutex.unlock();
                }
            };
            custom_lock cl(this, cv, lk);
            interruption_point();
            cv.wait(cl);
            interruption_point();
        }
};

thread_local interrupt_flag this_thread_interrupt_flag;

void interruption_point()
{
    if(this_thread_interrupt_flag.is_set())
    {
        throw std::logic_error("this thread interrupted");
    }
}

template<typename Lockable>
void interruptible_wait(std::condition_variable_any& cv, Lockable& lk) {
    this_thread_interrupt_flag.wait(cv, lk);
}

class Thread {
    private:
        std::thread internal_thread;
        interrupt_flag* flag;

    public:
        template<typename FunctionType>
        Thread(FunctionType f) {
            std::promise<interrupt_flag*> p;
            internal_thread = std::thread([f, &p] {
                p.set_value(&this_thread_interrupt_flag);
                f();
            });
            flag = p.get_future().get();
        }

        virtual ~Thread () = default;

    public:
        void join() {
            return internal_thread.join();
        }

        void detach() {
            return internal_thread.detach();
        }

        bool joinable() const {
            return internal_thread.joinable();
        }

        void interrupt() {
            if(flag) {
                flag->set();
            }
        }
};

#endif /* THREAD_H */
