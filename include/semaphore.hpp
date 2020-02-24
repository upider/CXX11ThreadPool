#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

#include <semaphore.h>
#include <ctime>

class Semaphore {
    private:
        sem_t sem_;

    public:
        int post() {
            return sem_post(&sem_);
        }

        int wait() {
            return sem_wait(&sem_);
        }

        int tryWait() {
            return sem_trywait(&sem_);
        }

        int timedWait(unsigned int seconds, unsigned int nanoseconds) {
            timespec ts{seconds, nanoseconds};
            return sem_timedwait(&sem_, &ts);
        }

    public:
        Semaphore(unsigned int initSize = 0) {
            sem_init(&sem_, 0, initSize);
        }
        virtual ~Semaphore () {
            sem_destroy(&sem_);
        }

        Semaphore(const Semaphore&) = delete;
        Semaphore(const Semaphore&&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&&) = delete;
};

#endif /* SEMAPHORE_HPP */
