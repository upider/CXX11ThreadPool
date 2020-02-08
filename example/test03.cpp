//测试submit方法,toString方法
#include <iostream>
#include <chrono>
#include "runnable.hpp"
#include "threadpool.hpp"

int hello(int v) {
    return 100;
}

int main(void)
{
    ThreadPoolExecutor tpe(1, 1);
    std::future<int> f;
    std::future<int> f2;
    for (int i = 0; i < 1; ++i) {
        f = tpe.submit(std::bind(&hello, 100));
        f2 = tpe.submit([]() -> int{
            std::cout << std::this_thread::get_id() << std::endl;
            return 10000;
        });
    }

    std::cout << "f = " << f.get() << std::endl;
    std::cout << "f2 = " << f2.get() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << tpe.toString() << std::endl;
    tpe.shutdown();
    //没有stop线程池不会退出
    tpe.stop();
    return 0;
}
