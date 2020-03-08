#include <iostream>
#include "rwlock.hpp"
#include "thread.hpp"

int main(void)
{
    RWLock rwLock;
    int x = 0;

    Thread tRead([&]() {
        for(;;) {
            rwLock.rdLock();
            std::cout << x << std::endl;
            rwLock.unlock();
            if (x >= 999) {
                break;
            }
        }
    }, "read_thread");

    Thread tWrite([&]() {
        for(int i = 0; i < 1000; i++) {
            rwLock.wrLock();
            x++;
            std::cout << "write operation x++" << std::endl;
            rwLock.unlock();
        }
    }, "write_thrad");

    tRead.start();
    tWrite.start();

    sleep(3);

    return 0;
}
