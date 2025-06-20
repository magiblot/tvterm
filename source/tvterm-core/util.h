#ifndef TVTERM_UTIL_H
#define TVTERM_UTIL_H

#include <tvision/tv.h>

// https://stackoverflow.com/a/60166119

template <class T, T>
struct static_wrapper {};

template <class T, class R, class... Args, R(T::*func)(Args...)>
struct static_wrapper<R(T::*)(Args...), func>
{
    static R invoke(Args... args, void *user)
    {
        return (((T*) user)->*func)(static_cast<Args&&>(args)...);
    }
};

#define _static_wrap(func) (&static_wrapper<decltype(func), func>::invoke)

inline constexpr uint32_t utf8To32(TStringView s)
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

#endif // TVTERM_UTIL_H
