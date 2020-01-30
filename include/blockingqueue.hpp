#ifndef BLOCKINGQUEUE_HPP
#define BLOCKINGQUEUE_HPP

#include <mutex>
#include <queue>
#include <condition_variable>
#include <iostream>
#include <cassert>

#include "runnable.hpp"

template <typename T>
/// @brief BlockingQueue
class BlockingQueue {
    public:
        using MutexLockGuard = std::lock_guard<std::mutex>;

        BlockingQueue()
            : _mutex(),
              _notEmpty(),
              _queue() {}

        BlockingQueue(std::initializer_list<T> args)
            : _mutex(),
              _notEmpty(),
              _queue{args} {}

        BlockingQueue(const BlockingQueue&);
        BlockingQueue(const BlockingQueue&&);
        BlockingQueue& operator=(const BlockingQueue &);
        BlockingQueue& operator=(const BlockingQueue&&);

        void put(const T &x) {
            std::lock_guard<std::mutex> lk(_mutex);
            _queue.push(x);
            _notEmpty.notify_one();
        }

        void put(const T &&x) {
            std::lock_guard<std::mutex> lk(_mutex);
            _queue.push(std::move(x));
            _notEmpty.notify_one();
        }

        T take() {
            std::unique_lock<std::mutex> lk(_mutex);
            _notEmpty.wait(lk, [this] {  return !this->_queue.empty(); });
            assert(!_queue.empty());

            T front(std::move(_queue.front()));
            _queue.pop();

            return  front;
        }

        void wait_and_pop(T& value) {
            std::unique_lock<std::mutex> lk(_mutex);
            _notEmpty.wait(lk, [this] {return !_queue.empty();});
            value = _queue.front();
            _queue.pop();
        }

        bool try_pop(T& value) {
            std::lock_guard<std::mutex> lk(_mutex);
            if(_queue.empty())
                return false;
            value = std::move(_queue.front());
            _queue.pop();
            return true;
        }

        size_t size() const {
            std::lock_guard<std::mutex> lk(_mutex);
            return _queue.size();
        }

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
