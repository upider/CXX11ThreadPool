#ifndef FUNCTOR_WRAPPER_HPP
#define FUNCTOR_WRAPPER_HPP

#include <utility>

/**
 * @brief 函数包装器虚基类
*/
struct Functor_base {
    Functor_base() = default;
    virtual void call() = 0;
    virtual ~Functor_base() {}
};

template<typename F>
struct Functor_t: Functor_base {
    /**
     * @brief functor_t 构造函数
     *
     * @param std::move(f) 包装的函数
     */
    Functor_t(F&& f): f_(std::move(f)) {}
    /**
     * @brief call 执行被包装的函数
     */
    void call() override {
        f_();
    }
    F f_;
};
#endif /* FUNCTOR_WRAPPER_HPP */
