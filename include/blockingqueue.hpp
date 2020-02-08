#ifndef BLOCKINGQUEUE_HPP
#define BLOCKINGQUEUE_HPP

#include <mutex>
#include <queue>
#include <condition_variable>
#include <iostream>
#include <cassert>

#include "runnable.hpp"

template <typename T>
/// @brief BlockingQueue 阻塞队列(FIFO)
class BlockingQueue {
    public:
        /**
         * @brief BlockingQueue 默认构造函数
         */
        BlockingQueue()
            : _mutex(),
              _notEmpty(),
              _queue() {}

        /**
         *  @brief BlockingQueue 初始化列表构造函数
         *
         *  @param args 初始化列表
         */
        BlockingQueue(std::initializer_list<T>& args)
            : _mutex(),
              _notEmpty(),
              _queue{args} {}

        /**
         * @brief BlockingQueue 拷贝构造
         *
         * @param rh BlockingQueue& 阻塞队列
         */
        BlockingQueue(const BlockingQueue&);

        /**
         * @brief BlockingQueue 复制构造函数
         *
         * @param rh BlockingQueue 阻塞队列
         */
        BlockingQueue(const BlockingQueue&& rh);

        /**
         * @brief operator= 赋值运算符
         *
         * @return rh BlockingQueue 阻塞队列的引用
         */
        BlockingQueue& operator=(const BlockingQueue &);

        /**
         * @brief operator= 赋值运算符
         *
         * @param rh BlockingQueue 阻塞队列
         *
         * @return BlockingQueue 阻塞队列的引用
         */
        BlockingQueue& operator=(const BlockingQueue&&);

        /**
         * @brief put 插入
         *
         * @param x
         */
        void put(const T &x) {
            std::lock_guard<std::mutex> lk(_mutex);
            _queue.push(x);
            _notEmpty.notify_one();
        }

        /**
         * @brief put 插入
         *
         * @param x
         */
        void put(const T&& x) {
            std::lock_guard<std::mutex> lk(_mutex);
            _queue.push(std::move(x));
            _notEmpty.notify_one();
        }

        /**
         * @brief take 弹出队列元素
         *
         * @return 队列元素
         */
        T take() {
            std::unique_lock<std::mutex> lk(_mutex);
            _notEmpty.wait(lk, [this] {  return !this->_queue.empty(); });
            assert(!_queue.empty());

            T front(std::move(_queue.front()));
            _queue.pop();

            return  front;
        }

        /**
         * @brief wait_and_pop 弹出队列元素
         *
         * @param value 弹出元素赋值对象
         */
        void wait_and_pop(T& value) {
            std::unique_lock<std::mutex> lk(_mutex);
            _notEmpty.wait(lk, [this] {return !_queue.empty();});
            value = _queue.front();
            _queue.pop();
        }

        /**
         * @brief wait_and_pop 弹出队列元素
         *
         * @param value 弹出元素赋值对象
         */
        bool try_pop(T& value) {
            std::lock_guard<std::mutex> lk(_mutex);
            if(_queue.empty())
                return false;
            value = std::move(_queue.front());
            _queue.pop();
            return true;
        }

        /**
         * @brief size 返回元素个数
         *
         * @return 元素个数
         */
        size_t size() const {
            std::lock_guard<std::mutex> lk(_mutex);
            return _queue.size();
        }

        /**
         * @brief is_empty 判断队列是否为空
         *
         * @return true - 队列为空
         */
        bool is_empty() const {
            std::lock_guard<std::mutex> lk(_mutex);
            return _queue.empty();
        }

    private:
        mutable std::mutex _mutex;
        std::condition_variable _notEmpty;
        std::queue<T> _queue;
};


template <typename T>
BlockingQueue<T>::BlockingQueue(const BlockingQueue<T>& rh)
    : _mutex(),
      _notEmpty() {
    std::lock_guard<std::mutex> lk(_mutex);
    this->_queue = rh._queue;
}

template <typename T>
BlockingQueue<T>::BlockingQueue(const BlockingQueue<T>&& rh)
    : _mutex(),
      _notEmpty() {
    std::lock_guard<std::mutex> lk(_mutex);
    this->_queue = rh._queue;
}
template <typename T>
BlockingQueue<T>& BlockingQueue<T>::operator=(const BlockingQueue<T>&& rh) {
    std::lock_guard<std::mutex> lk(_mutex);
    this->_queue = rh._queue;
}

template <typename T>
BlockingQueue<T>& BlockingQueue<T>::operator=(const BlockingQueue<T> & rh) {
    std::lock_guard<std::mutex> lk(_mutex);
    this->_queue = rh._queue;
}

#endif /* BLOCKINGQUEUE_HPP */
