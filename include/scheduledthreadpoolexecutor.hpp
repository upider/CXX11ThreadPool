#ifndef SCHEDULEDTHREADPOOLEXECUTOR_H
#define SCHEDULEDTHREADPOOLEXECUTOR_H

#include "threadpoolexecutor.hpp"

/**
 * @brief 定时任务调度线程池
 */
class ScheduledThreadPoolExecutor: public ThreadPoolExecutor {
    public:
        ScheduledThreadPoolExecutor();
        virtual ~ScheduledThreadPoolExecutor ();
};

#endif /* SCHEDULEDTHREADPOOLEXECUTOR_H */
