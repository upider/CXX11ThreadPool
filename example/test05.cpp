#include <iostream>
#include <chrono>
#include "runnable.hpp"
#include "threadpool.hpp"

class R: public Runnable {
    public:
        virtual void operator()() {
            std::cout << name  << std::endl;
        }

    private:
        std::string name = "RRRRRRRRRRRRRRRRRRRRR";
};

int main(void)
{
    R r1, r2;

    ThreadPoolExecutor tpe(1, 1);
    std::future<int> f;
    std::future<int> f2;
    for (int i = 0; i < 1; ++i) {
        tpe.submit(r1);
        tpe.submit([]() {
            std::cout << std::this_thread::get_id() << std::endl;
        });
    }

    tpe.interruptIdleWorkers();

    //tpe.stop();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << tpe.toString() << std::endl;
    return 0;
}
