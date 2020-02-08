//使用BlockingQueue<Runnable::sptr>替换
//BlockingQueue<Runnable>
//Runnable测试
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

int main(void)
{
    ThreadPoolExecutor tpe(1, 2);
    std::cout << "main--" << syscall(__NR_gettid)  << std::endl;

    Runnable r1([]() {
        std::cout << "r1" << std::endl;
    });
    r1();

    //拷贝构造不会复制functor_
    //通过构造函数传递的lambda不会被拷贝
    Runnable r2(r1);
    r2();

    //使用shared_ptr可以保存运行后的结果
    auto rsp = std::make_shared<R>();
    tpe.execute(rsp);

    sleep(1);
    //刚刚提交的任务有可能不会立即执行,需要等待
    std::cout << "R-name: " << rsp->getName() << std::endl;
    tpe.releaseNonCoreThreads();
    tpe.shutdown();
    tpe.stop();
    std::cout << tpe.toString() << std::endl;
    return 0;
}
