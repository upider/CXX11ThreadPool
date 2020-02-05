#include <functional>
#include <vector>
#include <iostream>
#include "thread.hpp"

class Pool {
    public:
        void start() {
            std::bind(&Pool::runInThread, this);
        }

        void runInThread() {
            std::cout << syscall(__NR_gettid) << std::endl;
        }

    private:
        std::vector<Thread::sptr> v;
};
