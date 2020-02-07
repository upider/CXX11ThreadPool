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
    wsThreadPool.submit([]() {
        std::cout << syscall(__NR_gettid) << std::endl;
    }, false);
    //sleep(2);
    std::cout << wsThreadPool.toString() << std::endl;
    wsThreadPool.shutdown();
    wsThreadPool.stop();
    std::cout << wsThreadPool.toString() << std::endl;
    return 0;
}
