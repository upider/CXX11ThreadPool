#ifndef WORKSTEALINGPOOLEXECUTOR_H
#define WORKSTEALINGPOOLEXECUTOR_H

#include <AdjustableThreadPoolExecutor>

class WorkStealingPoolExecutor: public AdjustableThreadPoolExecutor {
    public:
        WorkStealingPoolExecutor();
        virtual ~WorkStealingPoolExecutor ();




};

#endif /* WORKSTEALINGPOOLEXECUTOR_H */
