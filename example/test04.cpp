#include <iostream>
#include <chrono>
#include "runnable.hpp"
#include "threadpool.hpp"

class R: public Runnable {
    public:
        virtual void operator()() {
            std::cout << syscall(__NR_gettid)  << std::endl;
            //std::cout << "Runnable" << __LINE__ << std::endl;
        }

    private:
        std::string name = "RRRRRRRRRRRRRRRRRRRRR";
};

int main(void)
{
    std::cout << "main--" << syscall(__NR_gettid)  << std::endl;
    R r1, r2;
    RejectedExecutionHandler rj;

    //ThreadPoolExecutor tpe(1, 1, bq, rj);
    ThreadPoolExecutor tpe(1, 2);
    std::future<int> f;
    std::future<int> f2;
    for (int i = 0; i < 1; ++i) {
        tpe.submit(r1);
        f = tpe.submit([]()->int {
            //std::cout << "submit " << __LINE__ << std::endl;
            std::cout << syscall(__NR_gettid)  << std::endl;
            return 999;
        });
    }

    std::cout << tpe.toString() << std::endl;
    std::cout << f.get() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    tpe.submit([]()->int {
        //std::cout << "submit " << __LINE__ << std::endl;
        std::cout << syscall(__NR_gettid)  << std::endl;
        return 999;
    }, false);
    //shutdown test--应该跑出异常
    tpe.shutdown();
    std::cout << tpe.toString() << std::endl;
    /*    tpe.submit([]()->int {*/
    //std::cout << "submit " << __LINE__ << std::endl;
    //std::cout << syscall(__NR_gettid)  << std::endl;
    //return 999;
    /*});*/
    tpe.stop();

    sleep(1);
    std::cout << tpe.toString() << std::endl;
    return 0;
}
