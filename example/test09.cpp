//使用BlockingQueue<Runnable::sptr>替换
//BlockingQueue<Runnable>
#include <iostream>
#include <chrono>
#include "runnable.hpp"
#include "threadpool.hpp"

class R: public Runnable {
    public:
        virtual void operator()() {
            std::cout << "需要重写operator"  << std::endl;
            name += "l";
        }

    public:
        std::string getName()const {
            return name;
        }

    private:
        std::string name = "R";
};

void test(Runnable::sptr) {}
void test(Runnable&) {}

int main(void)
{
    ThreadPoolExecutor tpe(1, 2);
    std::cout << "main--" << syscall(__NR_gettid)  << std::endl;

    auto rsp = std::make_shared<R>();
    test(rsp);
    tpe.execute(rsp);

    tpe.submit([]() ->int {
        std::cout << "submit" << std::endl;
        return 999;
    }, false);

    sleep(1);
    //刚刚提交的任务有可能不会立即执行,需要等待
    std::cout << "R-name: " << rsp->getName() << std::endl;
    tpe.releaseNonCoreThreads();
    tpe.shutdown();
    tpe.stop();
    std::cout << tpe.toString() << std::endl;
    return 0;
}
