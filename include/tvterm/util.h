#ifndef TVTERM_UTIL_H
#define TVTERM_UTIL_H

#define Uses_TDialog
#define Uses_TProgram
#define Uses_TDeskTop
#include <tvision/tv.h>
#include <string_view>
#include <utility>
#include <new>

inline ushort execDialog(TDialog *d)
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
int fd_unset_flags(int fd, int flags);

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

inline constexpr uint32_t utf8To32(std::string_view s)
{
    constexpr uint32_t error = 0xFFFD; // "�".
    switch (s.size())
    {
        case 1:
            if (((uchar) s[0] & 0b1000'0000) == 0)
                return s[0];
            return error;
        case 2:
            if ((((uchar) s[0] & 0b1110'0000) == 0b1100'0000) & (((uchar) s[1] & 0b1100'0000) == 0b1000'0000))
                return ((s[0] & 0b0001'1111) << 6)  |  (s[1] & 0b0011'1111);
            return error;
        case 3:
            if ( (((uchar) s[0] & 0b1111'0000) == 0b1110'0000) & (((uchar) s[1] & 0b1100'0000) == 0b1000'0000) &
                 (((uchar) s[2] & 0b1100'0000) == 0b1000'0000)
               )
                return ((s[0] & 0b0000'1111) << 12) | ((s[1] & 0b0011'1111) << 6)  |  (s[2] & 0b0011'1111);
            return error;
        case 4:
            if ( (((uchar) s[0] & 0b1111'1000) == 0b1111'0000) & (((uchar) s[1] & 0b1100'0000) == 0b1000'0000) &
                 (((uchar) s[2] & 0b1100'0000) == 0b1000'0000) & (((uchar) s[3] & 0b1100'0000) == 0b1000'0000)
               )
                return ((s[0] & 0b0000'1111) << 18) | ((s[1] & 0b0011'1111) << 12) | ((s[2] & 0b0011'1111) << 6) | (s[3] & 0b0011'1111);
            return error;
    }
    return 0;
}

inline constexpr uchar swapRedBlue(uchar c)
{
    uchar t1 = (c & 0b0000'0001) << 2;
    uchar t2 = (c & 0b0000'0100) >> 2;
    uchar t3 = (c & 0b0000'1010);
    return t1 | t2 | t3;
}

template <class T>
union managed_lifetime
{
private:

    T object;

public:

    // No-op constructor and destructor.
    constexpr managed_lifetime() {};
    ~managed_lifetime() {};

    // On-demand construction and destruction.
    template<class... Args>
    void construct(Args&&... args)
    {
        new ((void *) &object) T(std::forward<Args>(args)...);
    }

    void destroy()
    {
        object.~T();
    }

    // Don't even try (it would require knowing whether the object is alive or not).
    managed_lifetime& operator=(managed_lifetime&&) = delete;
    managed_lifetime& operator=(const managed_lifetime&) = delete;

    // Accessors.
    constexpr operator T&()
    {
        return object;
    }

    constexpr operator const T&() const
    {
        return object;
    }

    constexpr T* operator->()
    {
        return &object;
    }

    constexpr const T* operator->() const
    {
        return &object;
    }

    constexpr T& get()
    {
        return operator T&();
    }

    constexpr const T& get() const
    {
        return operator T&();
    }


};

#endif // TVTERM_UTIL_H
