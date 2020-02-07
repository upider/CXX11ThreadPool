//测试submit方法
#include <iostream>
#include <chrono>
#include "runnable.hpp"
#include "threadpool.hpp"

int main(void)
{
    ThreadPoolExecutor tpe(1, 5);
    //tpe.startCoreThreads();
    std::future<int> f;
    std::future<int> f2;
    for (int i = 0; i < 1; ++i) {
        tpe.submit([]() {
            std::cout << __LINE__ << "--tid : " << syscall(__NR_gettid) << std::endl;
        });
    }

    //std::this_thread::sleep_for(std::chrono::seconds(1));
    //tpe.releaseNonCoreThreads();
    //std::this_thread::sleep_for(std::chrono::seconds(1));

    tpe.submit([]() {
        std::cout << __LINE__ << "--tid : " << syscall(__NR_gettid) << std::endl;
        //std::this_thread::sleep_for(std::chrono::seconds(2));
        //std::cout << std::this_thread::get_id() << std::endl;
    });
    tpe.submit([]() {
        std::cout << __LINE__ << "--tid : " << syscall(__NR_gettid) << std::endl;
        //std::this_thread::sleep_for(std::chrono::seconds(2));
        //std::cout << std::this_thread::get_id() << std::endl;
    });

    //tpe.stop();
    //std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << tpe.toString() << std::endl;
    return 0;
}
