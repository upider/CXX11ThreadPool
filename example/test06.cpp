#include <iostream>
#include <chrono>
#include "thread.hpp"

int main(void)
{
    Thread t([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "hello" << std::endl;
    });
    t.join();

    try {
        t.interrupt();
    } catch(std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 0;
}
