#ifndef TVTERM_MUTEX_H
#define TVTERM_MUTEX_H

#include <tvterm/debug.h>

#include <utility>
#include <mutex>

namespace tvterm
{

template <class T>
class TMutex
{
    T t;
    std::mutex m;

public:

    TMutex() = default;

    template <class... Args>
    TMutex(Args&&... args) :
        t {std::forward<Args&&>(args)...}
    {
    }

    template <class Func>
    auto lock(Func &&func)
    {
        std::lock_guard<std::mutex> lk {m};
        return func(t);
    }

    T &get()
    {
        return t;
    }

    const T &get() const
    {
        return t;
    }

};

template <>
class TMutex<void>
{
    std::mutex m;

public:

    TMutex() = default;

    template <class Func>
    auto lock(Func &&func)
    {
        std::lock_guard<std::mutex> lk {m};
        return func();
    }

};

} // namespace tvterm

#endif // TVTERM_MUTEX_H
