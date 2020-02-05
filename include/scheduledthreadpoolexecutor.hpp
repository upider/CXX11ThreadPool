#ifndef SCHEDULEDTHREADPOOLEXECUTOR_H
#define SCHEDULEDTHREADPOOLEXECUTOR_H

#include "threadpoolexecutor.hpp"

class ScheduledThreadPoolExecutor: public ThreadPoolExecutor {
    public:
        ScheduledThreadPoolExecutor();
        virtual ~ScheduledThreadPoolExecutor ();
};

#endif /* SCHEDULEDTHREADPOOLEXECUTOR_H */
