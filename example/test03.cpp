#include <iostream>
#include <chrono>
#include "runnable.hpp"
#include "threadpool.hpp"

int hello(int v) {
    return 100;
}

class RR: public Runnable {
    public:
        virtual void operator()() {
            std::cout << name  << std::endl;
        }

    private:
        std::string name = "RRRRRRRRRRRRRRRRRRRRR";
};

RR rr;
RR rrr;
RR rrrr;
RR rrrrr;
RR rrrrrr;

int main(void)
{
    BlockingQueue<Runnable> bq;
    RejectedExecutionHandler rj;

    ThreadPoolExecutor tpe(1, 1, 1, TimeUnit::seconds, bq, rj);
    std::future<int> f;
    std::future<int> f2;
    for (int i = 0; i < 1; ++i) {
        /*tpe.execute(rr);*/
        //tpe.execute(rrr);
        //tpe.execute(rrrr);
        /*tpe.execute(rrrrr);*/
        //tpe.execute(rrrrrr);
        //tpe.submit([]() {
        //    std::cout << std::this_thread::get_id() << std::endl;
        //});
        /*tpe.submit(rrrrr);*/
        f = tpe.submit(std::bind(&hello, 100));
        //f2 = tpe.submit([]() -> int{
        //    std::cout << std::this_thread::get_id() << std::endl;
        //    return 10000;
        /*});*/
    }

    std::cout << "f = " << f.get() << std::endl;
    //std::cout << "f2 = " << f2.get() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << tpe.toString() << std::endl;
    return 0;
}
