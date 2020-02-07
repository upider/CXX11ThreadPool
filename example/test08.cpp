//测试workstealingpoolexecutor
#include "workstealingpoolexecutor.hpp"

int main(void)
{
    WorkStealingPoolExecutor wsThreadPool(1, 2);

    wsThreadPool.submit([]() {
        std::cout << syscall(__NR_gettid) << std::endl;
    });
    wsThreadPool.submit([]() {
        std::cout << syscall(__NR_gettid) << std::endl;
    });
    for (int i = 0; i < 10; ++i) {
        wsThreadPool.submit([]() {
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
