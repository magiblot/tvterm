#ifndef TVTERM_UTIL_H
#define TVTERM_UTIL_H

#define Uses_TDialog
#define Uses_TProgram
#define Uses_TDeskTop
#include <tvision/tv.h>
#include <string_view>

ushort execDialog(TDialog *d)
{
    TView *p = TProgram::application->validView(d);
    if (p)
    {
        ushort result = TProgram::deskTop->execView(p);
        TObject::destroy(p);
        return result;
    }
    return cmCancel;
}

int fd_set_flags(int fd, int flags);

// https://stackoverflow.com/a/60166119

template <auto T>
struct static_wrapper {};

template <class T, class R, class... Args, R(T::*func)(Args...)>
struct static_wrapper<func>
{
    static R invoke(Args... args, void *user)
    {
        return (((T*) user)->*func)(args...);
    }
};

template <auto T>
constexpr auto static_wrap = &static_wrapper<T>::invoke;

#endif // TVTERM_UTIL_H
