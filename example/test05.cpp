#include <iostream>
#include <chrono>
#include "runnable.hpp"
#include "threadpool.hpp"

class R: public Runnable {
    public:
        virtual void operator()() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::cout << name  << std::this_thread::get_id() << std::endl;
        }

    private:
        std::string name = "RRRRRRRRRRRRRRRRRRRRR";
};

int main(void)
{
    R r1, r2;

    ThreadPoolExecutor tpe(1, 5);
    std::future<int> f;
    std::future<int> f2;
    for (int i = 0; i < 1; ++i) {
        tpe.submit(r1);
        tpe.submit([]() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::cout << std::this_thread::get_id() << std::endl;
        });
    }

    //std::this_thread::sleep_for(std::chrono::seconds(1));
    tpe.releaseIdleWorkers();
    //std::this_thread::sleep_for(std::chrono::seconds(1));

    tpe.submit([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << std::this_thread::get_id() << std::endl;
    });

    //tpe.stop();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << tpe.toString() << std::endl;
    return 0;
}
