//ScheduledThreadPoolExecutor测试
//注意:任务结束关闭线程池
#include <iostream>
#include "scheduledthreadpoolexecutor.hpp"

class R: public Runnable {
    public:
        void operator()() {
            std::cout << "this is R -> Runnable" << std::endl;
        }
};

int test() {
    std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << "4th " << std::asctime(std::localtime(&tt));
    return 99999;
}

/**
 * @brief 重载TimerTask 完成任务
 */
struct TT: public TimerTask {
    public:
        TT(std::chrono::nanoseconds initDelay, std::chrono::nanoseconds delay, bool fixedRate)
            : TimerTask(initDelay, delay, fixedRate, []() {}) {}
        void operator()() {
            std::cout << "this is TT " << x++ << std::endl;
        }
    private:
        int x = 0;
};

int main(void)
{
    ScheduledThreadPoolExecutor tpe(3, "STPE");
    tpe.preStartCoreThreads();

    //传入shared_ptr<TimerTask>
    auto lambda = []() {
        std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cout << "0st " << std::asctime(std::localtime(&tt));
    };
    auto tt = std::make_shared<TimerTask>(std::chrono::nanoseconds(0), std::chrono::seconds(1), false, lambda);
    tpe.schedule(tt);

    //传入shared_ptr<TT> TT:public TimerTask
    auto t = std::make_shared<TT>(std::chrono::nanoseconds(0), std::chrono::seconds(1), false);
    tpe.schedule(t);

    //传入重载的Runnable对象
    R r1;
    tpe.schedule(r1, std::chrono::seconds(1));

    //传入lambda
    tpe.schedule([]() ->int {
        std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cout << "1st " << std::asctime(std::localtime(&tt));
        return 999;
    }, std::chrono::seconds(1));

    tpe.schedule([]() ->int {
        std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cout << "3rd " << std::asctime(std::localtime(&tt));
        return 999;
    }, std::chrono::seconds(3));

    tpe.schedule([]() ->int {
        std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cout << "2nd " << std::asctime(std::localtime(&tt));
        return 999;
    }, std::chrono::seconds(2));

    //在ScheduledThreadPoolExecutor中使用std::future
    std::packaged_task<std::string()> p([]() ->std::string {
        std::ostringstream os;
        std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cout << "4th " << std::asctime(std::localtime(&tt));
        os << "4th schedule's the last time is " << std::asctime(std::localtime(&tt));
        return os.str();
    });
    std::future<std::string> f(p.get_future());
    tpe.scheduleAtFixedRate([&]() {
        p();
        p.reset();//每次执行后都要取消关联才能再次执行
    }, std::chrono::seconds(2), std::chrono::seconds(2));

    tpe.scheduleAtFixedRate(test, std::chrono::seconds(2), std::chrono::seconds(2));

    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << f.get() << std::endl;
    tpe.stop();
    return 0;
}
