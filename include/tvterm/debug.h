#ifndef TVTERM_DEBUG_H
#define TVTERM_DEBUG_H

#include <iostream>


namespace debug
{

    // Set a breakpoint in debug::breakable and call debug_breakable()
    // to trigger it.
    void breakable();

    // Call through volatile pointer is never inlined.
    extern void (* volatile const breakable_ptr)();

} // namespace debug

inline void debug_breakable()
{
    (*debug::breakable_ptr)();
}

class DebugCout
{

    struct NullStreambuf : public std::streambuf
    {
        int_type overflow(int_type ch) override { return ch; }
    };

    bool enabled;
    NullStreambuf nbuf;
    std::ostream nout;

    DebugCout();

public:

    static DebugCout instance;

    operator std::ostream&();

    template <class T>
    std::ostream& operator<<(const T &t);

};

static DebugCout &dout = DebugCout::instance;

inline DebugCout::operator std::ostream&()
{
    if (enabled)
        return std::cerr;
    return nout;
}

template <class T>
inline std::ostream& DebugCout::operator<<(const T &t)
{
    return operator std::ostream&() << t;
}

#define Uses_TRect
#include <tvision/tv.h>

inline std::ostream &operator<<(std::ostream &os, const TRect &r)
{
    return os << "TRect {"
                ".a.x = " << r.a.x << ", .a.y = " << r.a.y << ", "
                ".b.x = " << r.b.x << ", .b.y = " << r.b.y << " }";
}

#include <vterm.h>
#include <string_view>

inline std::ostream &operator<<(std::ostream &os, const VTermPos &a)
{
    return os << "VTermPos { .row = " << a.row << ", .col = " << a.col << " }";
}

inline std::ostream &operator<<(std::ostream &os, const VTermRect &a)
{
    return os << "VTermRect {"
                ".start_col = " << a.start_col << ", .end_col = " << a.end_col << ", "
                ".start_row = " << a.start_row << ", .end_row = " << a.end_row << " }";
}

inline std::ostream &operator<<(std::ostream &os, const VTermProp &a)
{
    std::string_view sv;
    switch (a)
    {
        case VTERM_PROP_CURSORVISIBLE: sv = "VTERM_PROP_CURSORVISIBLE"; break;
        case VTERM_PROP_CURSORBLINK: sv = "VTERM_PROP_CURSORBLINK"; break;
        case VTERM_PROP_ALTSCREEN: sv = "VTERM_PROP_ALTSCREEN"; break;
        case VTERM_PROP_TITLE: sv = "VTERM_PROP_TITLE"; break;
        case VTERM_PROP_ICONNAME: sv = "VTERM_PROP_ICONNAME"; break;
        case VTERM_PROP_REVERSE: sv = "VTERM_PROP_REVERSE"; break;
        case VTERM_PROP_CURSORSHAPE: sv = "VTERM_PROP_CURSORSHAPE"; break;
        case VTERM_PROP_MOUSE: sv = "VTERM_PROP_MOUSE"; break;
        case VTERM_N_PROPS: sv = "VTERM_N_PROPS"; break;
    }
    return os << sv;
}

#endif // TVTERM_DEBUG_H
