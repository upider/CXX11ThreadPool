//测试BlockingQueue,RejectedExecutionHandler 构造函数
#include <iostream>
#include "threadpool.hpp"

int main(void)
{
    BlockingQueue<Runnable> bq{};
    RejectedExecutionHandler rj;
    return 0;
}
