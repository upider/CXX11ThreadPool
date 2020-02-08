//测试workstealingpoolexecutor
#include "workstealingthreadpoolexecutor.hpp"

int main(void)
{
    WorkStealingThreadPoolExecutor wsThreadPool(1, 3);
    wsThreadPool.keepNonCoreThreadAlive(true);

    wsThreadPool.submit([]() {
        std::cout << syscall(__NR_gettid) << std::endl;
    });
    wsThreadPool.submit([]() {
        std::cout << syscall(__NR_gettid) << std::endl;
    });
    for (int i = 0; i < 10; ++i) {
        wsThreadPool.submit([]() {
            sleep(1);
            std::cout << syscall(__NR_gettid) << std::endl;
        }, false);
    }
    std::cout << wsThreadPool.toString() << std::endl;
    wsThreadPool.shutdown();
    sleep(2);
    wsThreadPool.stop();
    std::cout << wsThreadPool.toString() << std::endl;
    return 0;
}
