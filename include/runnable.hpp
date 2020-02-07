#ifndef RUNNABLE_HPP
#define RUNNABLE_HPP

#include <memory>

/// @brief Runnable interface 需要重写operator()
class Runnable {
    public:
        using sptr = std::shared_ptr<Runnable>;

        template<typename F>
        /**
         * @brief Runnable 构造函数
         *
         * @param new functor_t(std::move(f))
         */
        Runnable(F&& f): functor_(new functor_t<F>(std::move(f))) {}

        /**
         * @brief Runnable 复制构造
         *
         * @param rth Runnable右值引用
         */
        Runnable(Runnable && rth) : functor_(std::move(rth.functor_)) {}

        template <typename F = Runnable>
        /**
         * @brief Runnable 拷贝构造不会复制functor_
         *
         * @param f Runnable引用
         */
        Runnable(F& f) {}

        /**
         * @brief operator= 复制
         *
         * @param rth 被复制的Runnable
         *
         * @return Runnable&
         */
        Runnable & operator=(Runnable && rth) {
            functor_ = std::move(rth.functor_);
            return *this;
        }

        /**
         * @brief Runnable 默认构造
         */
        Runnable() = default;
        /**
         * @brief ~Runnable 析构函数
         */
        virtual ~Runnable() = default;

        /**
         * @brief operator() 重载实现操作
         */
        virtual void operator()() {
            if (functor_ != nullptr) {
                functor_->call();
            }
        }

        /**
         * @brief empty 判断内部的函数包装器是否为空
         *
         * @return bool true-空
         */
        bool empty() const {
            return functor_ == nullptr;
        }

    public:
        template<typename F>
        Runnable(Runnable & rth) = delete;
        Runnable& operator=(Runnable& rth) = delete;

    private:
        struct functor_base {
            functor_base() = default;
            virtual void call() = 0;
            virtual ~functor_base() {}
        };

        template<typename F>
        struct functor_t: functor_base {
            functor_t(F&& f): _f(std::move(f)) {}
            void call() override {
                _f();
            }
            F _f;
        };

        std::unique_ptr<functor_base> functor_;
};

#endif /* RUNNABLE_HPP */
