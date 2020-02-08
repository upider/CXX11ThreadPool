//测试operator()()
#include <iostream>
#include <chrono>
#include "runnable.hpp"
#include "threadpool.hpp"

class R: public Runnable {
    public:
        virtual void operator()() {
            std::cout << "Runnable" << "--R" << std::endl;
        }
};

void test(int a) {
    std::cout << "a = " << a << std::endl;
}

int main(void)
{
    std::cout << "main--" << syscall(__NR_gettid)  << std::endl;

    R r1;
    auto r2 = std::make_shared<R>();

    ThreadPoolExecutor tpe(1, 2);
    for (int i = 0; i < 1; ++i) {
        tpe.submit([]() {
            std::cout << syscall(__NR_gettid)  << std::endl;
        });
        tpe.submit([]()->int {
            std::cout << syscall(__NR_gettid)  << std::endl;
            return 999;
        });
    }

    //测试std::bind
    Runnable r3(std::bind(test, 10));
    r3();

    //测试execute--execute对Runnable使用std::move,
    //原来的Runnable的对象将会不存在
    //对于std::shared_ptr<Runnable>不会使用std::move
    tpe.execute(r1);
    //无效操作
    r1();
    //使用std::shared_ptr<Runnable>
    tpe.execute(r2);
    //有效操作
    r2->operator()();

    tpe.submit([]() ->int {
        std::cout << syscall(__NR_gettid)  << std::endl;
        std::cout << __FILE__ << "-" << __LINE__ << std::endl;
        return 999;
    }, false);

    sleep(1);
    tpe.shutdown();
    tpe.stop();
    std::cout << tpe.toString() << std::endl;
    return 0;
}
