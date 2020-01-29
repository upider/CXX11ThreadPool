#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

int main(void)
{
    std::vector<std::thread> threads;

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([]() {
            std::cout << "thread id = " << std::this_thread::get_id() << std::endl;
        });
    }

    for (int i = 0; i < 4; ++i) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
