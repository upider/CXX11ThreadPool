//测试Thread类
#include <stdexcept>
#include <iostream>
#include "thread.hpp"

int main(void)
{
    Thread t([]() {
        std::cout << getCurrentThreadName() << std::endl;
    }, "Thread--t");

    Thread tt([]() {
        std::cout << getCurrentThreadName() << std::endl;
    }, "Thread--tt");

    tt.start();
    tt.join();

    t.start();

    t.detach();
    //t.join();
    std::cout << "hooooooo" << std::endl;

    sleep(3);
    return 0;
}
