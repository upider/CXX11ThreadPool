#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

#include <semaphore.h>
#include <ctime>

/**
 * @brief 信号量
 */
class Semaphore {
    private:
        sem_t sem_;

    public:
        /**
         * @brief post 信号量加一
         *
         * @return 错误码 0表示成功
         */
        int post() {
            return sem_post(&sem_);
        }

        /**
         * @brief wait 信号量减一
         *
         * @return 错误码 0表示成功
         */
        int wait() {
            return sem_wait(&sem_);
        }

        /**
         * @brief tryWait 信号量减一,立即返回
         *
         * @return 错误码 0表示成功
         */
        int tryWait() {
            return sem_trywait(&sem_);
        }

        /**
         * @brief timedWait 信号量减一
         *
         * @param seconds 等待时间－秒
         * @param nanoseconds 等待时间－纳秒
         *
         * @return 错误码 0表示成功
         */
        int timedWait(unsigned int seconds, unsigned int nanoseconds) {
            timespec ts{seconds, nanoseconds};
            return sem_timedwait(&sem_, &ts);
        }

    public:
        /**
         * @brief Semaphore 构造函数
         *
         * @param initSize 信号量初值
         */
        Semaphore(unsigned int initSize = 0) {
            sem_init(&sem_, 0, initSize);
        }
        /**
         * @brief ~Semaphore 析构函数
         */
        virtual ~Semaphore () {
            sem_destroy(&sem_);
        }

        Semaphore(const Semaphore&) = delete;
        Semaphore(const Semaphore&&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&&) = delete;
};

#endif /* SEMAPHORE_HPP */
