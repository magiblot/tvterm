#ifndef TVTERM_MUTEX_H
#define TVTERM_MUTEX_H

#include <mutex>

namespace tvterm
{

template <class T>
class Mutex
{
    std::mutex m;
    T item;

public:

    template <class... Args>
    Mutex(Args&&... args) :
        item(static_cast<Args &&>(args)...)
    {
    }

    template <class Func>
    auto lock(Func &&func)
    {
        std::lock_guard<std::mutex> lk {m};
        return func(item);
    }
};

} // namespace tvterm

#endif // TVTERM_MUTEX_H
