#ifndef RUNNABLE_HPP
#define RUNNABLE_HPP

#include <memory>

/// @brief Runnable interface 需要重写operator()
class Runnable {
    public:
        template<typename F>
        /**
         * @brief Runnable 构造函数
         *
         * @param new functor_t(std::move(f))
         */
        Runnable(F&& f): _functor_uptr(new functor_t<F>(std::move(f))) {}

        template<typename F>
        /**
         * @brief Runnable 构造函数
         *
         * @param new functor_t(std::move(f))
         */
        Runnable(F& f): _functor_uptr(new functor_t<F>(std::move(f))) {}

        /**
         * @brief Runnable 构造函数
         *
         * @param new functor_t(std::move(f))
         */
        Runnable(Runnable&& other): _functor_uptr(std::move(other._functor_uptr)) {}

        /**
         * @brief Runnable 构造函数
         *
         * @param new functor_t(std::move(f))
         */
        Runnable(const Runnable& other): _functor_uptr(other._functor_uptr) {}

        /**
         * @brief operator= 赋值运算符
         *
         * @param other
         *
         * @return Runnable&
         */
        Runnable& operator=(Runnable&& other) {
            _functor_uptr = std::move(other._functor_uptr);
            return *this;
        }

        /**
         * @brief operator= 赋值运算符
         *
         * @param other
         *
         * @return Runnable&
         */
        Runnable& operator=(const Runnable& other) {
            this->_functor_uptr = other._functor_uptr;
            return *this;
        }

        Runnable()  = default;
        ~Runnable() = default;

        /**
         * @brief operator() 重载实现操作
         */
        virtual void operator()() {
            _functor_uptr->call();
        }

        bool empty() const {
            return _functor_uptr == nullptr;
        }

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

        std::shared_ptr<functor_base> _functor_uptr;
};

#endif /* RUNNABLE_HPP */
