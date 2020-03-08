#ifndef RUNNABLE_HPP
#define RUNNABLE_HPP

#include <iostream>
#include <functional>
#include <memory>

#include "functor_wrapper.hpp"

/// @brief Runnable interface 重写operator()或传进lambda
//                            执行operator()可以运行任务
//                            千万不能把两个Runnable对象循环赋值
class Runnable {
    public:
        /**
         * @brief std::shared_ptr<Runnable>别名
         */
        using sptr = std::shared_ptr<Runnable>;

        template<typename F>
        /**
         * @brief Runnable 构造函数
         *
         * @param f lambda
         */
        Runnable(F&& f): functor_(new Functor_t<F>(std::move(f))) {}

        /**
         * @brief Runnable 复制构造
         *
         * @param rh Runnable右值引用
         */
        explicit Runnable(Runnable && rh): functor_(std::move(rh.functor_)) {}

        /**
         * @brief Runnable 拷贝构造不会复制functor_
         *
         * @param rh Runnable引用
         */
        explicit Runnable(Runnable & rh): functor_(std::move(rh.functor_)) {}

        /**
         * @brief operator= 复制
         *
         * @param rh 被复制的Runnable
         *
         * @return Runnable&
         */
        Runnable& operator=(Runnable && rh) {
            functor_ = std::move(rh.functor_);
            return *this;
        }

        /**
         * @brief operator= 复制
         *
         * @param rh 被复制的Runnable
         *
         * @return Runnable&
         */
        Runnable& operator=(Runnable & rh) {
            functor_ = std::move(rh.functor_);
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
            functor_.release();
        }

        /**
         * @brief empty 判断内部的函数包装器是否为空
         *
         * @return bool true-空
         */
        bool empty() const {
            return functor_ == nullptr;
        }

    protected:
        /**
         * @brief 函数包装器
         */
        std::unique_ptr<Functor_base> functor_;
};

#endif /* RUNNABLE_HPP */
