//测试releaseNonCoreThreads,以及线程动态增加方法
#include <iostream>
#include <chrono>
#include "runnable.hpp"
#include "threadpool.hpp"

int main(void)
{
    ThreadPoolExecutor tpe(1, 2);
    //tpe.preStartCoreThreads();
    tpe.submit([]() {
        std::cout << "task01" << "--tid : " << syscall(__NR_gettid) << std::endl;
    });
    tpe.keepNonCoreThreadAlive(true);

    //std::this_thread::sleep_for(std::chrono::seconds(1));

    tpe.submit([]() {
        std::cout << "task02" << "--tid : " << syscall(__NR_gettid) << std::endl;
    }, false);

    tpe.submit([]() {
        std::cout << "task03" << "--tid : " << syscall(__NR_gettid) << std::endl;
    }, false);

    //让非核心线程退出,
    //tpe.releaseNonCoreThreads();

    tpe.submit([]() {
        std::cout << "task04" << "--tid : " << syscall(__NR_gettid) << std::endl;
    }, false);

    tpe.submit([]() {
        std::cout << "task05" << "--tid : " << syscall(__NR_gettid) << std::endl;
    }, false);

    tpe.submit([]() {
        std::cout << "task06" << "--tid : " << syscall(__NR_gettid) << std::endl;
    });

    tpe.submit([]() {
        std::cout << "task07" << "--tid : " << syscall(__NR_gettid) << std::endl;
    });

    sleep(10);
    std::cout << tpe.toString() << std::endl;
    tpe.stop();
    return 0;
}
