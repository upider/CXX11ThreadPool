#ifndef WORKSTEALINGPOOLEXECUTOR_H
#define WORKSTEALINGPOOLEXECUTOR_H

#include "threadpoolexecutor.hpp"

class WorkStealingPoolExecutor: public ThreadPoolExecutor {
    public:
        WorkStealingPoolExecutor();
        virtual ~WorkStealingPoolExecutor ();
};

#endif /* WORKSTEALINGPOOLEXECUTOR_H */
