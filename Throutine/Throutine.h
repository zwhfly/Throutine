/*
 * Copyright (c) 2017 Wenhua Zheng (zwhfly@163.com)
 * Licensed under the MIT license
 */


#ifndef Throutine_H_
#define Throutine_H_


//Thread Based Coroutine


#include <future>
#include <functional>
#include <type_traits>
#include <utility>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <exception>
#include <stdexcept>
#include <any>


class ThroutineCore
{
    class Transmit
    {
    private:
        struct Void {};
    private:
        std::mutex mutex;
        std::condition_variable cv;
        std::any value;
        std::exception_ptr exception;
    public:
        template <typename A>
        void setValue(A && a)
        {
            {
                std::lock_guard<std::mutex> guard(mutex);
                if (value.has_value())
                    throw 1;//TODO
                value = std::forward<A>(a);
            }
            cv.notify_one();
        }
        void setValue()
        {
            return setValue(Void{});
        }
        void setException(std::exception_ptr e)
        {
            {
                std::lock_guard<std::mutex> guard(mutex);
                if (exception)
                {
                    ;//TODO
                }
                exception = std::move(e);
            }
            cv.notify_one();
        }
        template <typename R>
        std::enable_if_t<std::is_void<R>::value == false, R>
            get()
        {
            std::unique_lock<std::mutex> lock(mutex);
            for (;;)
            {
                if (value.has_value())
                {
                    R r(std::move(*std::any_cast<R>(&value)));
					value.reset();
                    return r;
                }
                if (exception)
                {
                    std::rethrow_exception(exception);
                    //keep the exception

					//Warning C4702
                    //throw 1;//guard
                }
                cv.wait(lock);
            }
        }
        template <typename R>
        std::enable_if_t<std::is_void<R>::value>
            get()
        {
            get<Void>();
        }
    };
private:
    struct Returned : std::runtime_error
    {
        Returned()
            :
            std::runtime_error("ThroutineCore returned.")
        {}
    };
    struct Aborted : std::runtime_error
    {
        Aborted()
            :
            std::runtime_error("ThroutineCore aborted.")
        {}
    };
private:
    Transmit forth;
    Transmit back;
    std::function<void ()> routine;
    std::future<void> exited;
public:
    template <typename F>
    ThroutineCore(F && f)
        :
        routine(std::forward<F>(f))
    {}
    ~ThroutineCore()
    {
        forth.setException(std::make_exception_ptr(Aborted{}));
        exited.wait();
    }
private:
    static ThroutineCore * & getThreadCore()
    {
        thread_local ThroutineCore * core = nullptr;
        return core;
    }
public:
    void start()
    {
        if (false == exited.valid())
        {
            exited = std::async(std::launch::async, [this]()
            {
                auto & core = getThreadCore();
                auto old_core = core;
                core = this;
                try
                {
                    routine();
                    back.setException(std::make_exception_ptr(Returned{}));
                }
                catch (...)
                {
                    back.setException(std::current_exception());
                }
                core = old_core;
            });
        }
        else
        {
            throw 1;//TODO
        }
    }
public:
    template <typename ... A>
    static void reply(A && ... a)
    {
        auto core = getThreadCore();
        if (!core)
            throw 1;//TODO
        static_assert(sizeof...(a) <= 1, "Multiple arguments are not supported.");
        core->back.setValue(std::forward<A>(a) ...);
    }
    template <typename R = void>
    static R accept()
    {
        auto core = getThreadCore();
        if (!core)
            throw 1;//TODO
        return core->forth.get<R>();
    }
public:
    template <typename ... A>
    void issue(A && ... a)
    {
        static_assert(sizeof...(a) <= 1, "Multiple arguments are not supported.");
        forth.setValue(std::forward<A>(a) ...);
    }
    template <typename R = void>
    R wait()
    {
        return back.get<R>();
    }
};

class Throutine : public ThroutineCore
{
public:
    template <typename F>
    Throutine(F && f)
        :
        ThroutineCore(std::forward<F>(f))
    {
        start();
    }
public:
    template <typename R = void, typename ... A>
    R call(A && ... a)
    {
        issue(std::forward<A>(a) ...);
        return wait<R>();
    }
    template <typename R = void, typename ... A>
    static R yield(A && ... a)
    {
        reply(std::forward<A>(a) ...);
        return accept<R>();
    }
};


#endif
