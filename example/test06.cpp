//测试Thread类
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
    std::cout << "t:" << t.getPid() << std::endl;
    std::cout << "tt:" << tt.getPid() << std::endl;

    sleep(3);
    std::cout << std::chrono::duration<double>(std::chrono::steady_clock::now() - tt.getLastActiveTime()).count() << std::endl;


    t.join();
    tt.join();
    return 0;
}
