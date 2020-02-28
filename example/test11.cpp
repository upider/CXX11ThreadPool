#include <iostream>
#include <future>
#include "thread.hpp"

int main(int argc, char *argv[])
{
    std::promise<int> promise;
    std::future<int> future(promise.get_future());

    Thread t([&promise]() {
        std::cout << "thread start.." << std::endl;
        promise.set_value(999);
    });

    t.start();
	//不需要手动释放线程资源
    //t.join();

    std::cout << future.get() << std::endl;
    return 0;
}
