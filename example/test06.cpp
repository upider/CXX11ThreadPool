#include <stdexcept>
#include <iostream>
#include "thread.hpp"

class MyThread : public Thread {
        virtual void run() override {
            //std::cout << std::this_thread::get_id() << std::endl;
            std::cout << getCurrentThreadName() << std::endl;
        }
};

int main(void)
{
    Thread tt([]() {
        sleep(3);
        std::cout << getCurrentThreadName() << std::endl;
    }, "Thread--t");
    //tt.setName("ttaaaaa");

    MyThread t;
    t.setName("Thread--tt");

    t.start();
    tt.start();
    std::cout << "tt--" << t.getCurrentPid() << std::endl;
    std::cout << "t--" << tt.getCurrentPid() << std::endl;

    std::cout << std::chrono::duration<double>(std::chrono::steady_clock::now() - tt.getLastActiveTime()).count() << std::endl;

    std::cout << tt.isIdle() << std::endl;
    std::cout << tt.isRunning() << std::endl;
    sleep(3);
    return 0;
}
