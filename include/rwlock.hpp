#ifndef RWLOCK_HPP
#define RWLOCK_HPP

#include <pthread.h>

class RWLock {
    private:
        pthread_rwlock_t rwlock_;

    public:
        int rdLock() {
            return pthread_rwlock_rdlock(&rwlock_);
        }

        int wrLock() {
            return pthread_rwlock_wrlock(&rwlock_);
        }

        int unlock() {
            return pthread_rwlock_unlock(&rwlock_);
        }

        int tryRdLock() {
            return pthread_rwlock_tryrdlock(&rwlock_);
        }

        int tryWrLock() {
            return pthread_rwlock_trywrlock(&rwlock_);
        }

    public:
        RWLock() {
            pthread_rwlock_init(&rwlock_, nullptr);
        }

        virtual ~RWLock () {
            pthread_rwlock_destroy(&rwlock_);
        }
};

#endif /* RWLOCK_HPP */
