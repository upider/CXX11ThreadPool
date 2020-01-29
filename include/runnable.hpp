#ifndef RUNNABLE_HPP
#define RUNNABLE_HPP

#include <memory>

/// @brief Runnable interface 需要重写operator()
class Runnable {
    public:
        template<typename F>
        Runnable(F&& f): _functor_uptr(new functor_t<F>(std::move(f))) {}

        template<typename F>
        Runnable(F& f): _functor_uptr(new functor_t<F>(std::move(f))) {}

        Runnable(Runnable&& other): _functor_uptr(std::move(other._functor_uptr)) {}

        Runnable(const Runnable& other): _functor_uptr(other._functor_uptr) {}

        Runnable& operator=(Runnable&& other) {
            _functor_uptr = std::move(other._functor_uptr);
            return *this;
        }

        Runnable& operator=(const Runnable& other) {
            this->_functor_uptr = other._functor_uptr;
            return *this;
        }

        Runnable()  = default;
        ~Runnable() = default;

        virtual void operator()() {
            _functor_uptr->call();
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
