#ifndef TVTERM_DEBUG_H
#define TVTERM_DEBUG_H

#include <iostream>

namespace tvterm
{
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

} // namespace tvterm

#include <tvterm/termadapt.h>

namespace tvterm
{

inline std::ostream &operator<<(std::ostream &os, const TerminalSurface::Range &r)
{
    return os << "{" << r.begin << ", " << r.end << "}";
}

} // namespace tvterm

#define Uses_TRect
#include <tvision/tv.h>

namespace tvterm
{

inline std::ostream &operator<<(std::ostream &os, TPoint p)
{
    return os << "{" << p.x << ", " << p.y << "}";
}

inline std::ostream &operator<<(std::ostream &os, const TRect &r)
{
    return os << "TRect {" << r.a.x << ", " << r.a.y << ", "
                           << r.b.x << ", " << r.b.y << "}";
}

} // namespace tvterm

#include <vterm.h>

namespace tvterm
{

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
    const char *s = "";
    switch (a)
    {
        case VTERM_PROP_CURSORVISIBLE: s = "VTERM_PROP_CURSORVISIBLE"; break;
        case VTERM_PROP_CURSORBLINK: s = "VTERM_PROP_CURSORBLINK"; break;
        case VTERM_PROP_ALTSCREEN: s = "VTERM_PROP_ALTSCREEN"; break;
        case VTERM_PROP_TITLE: s = "VTERM_PROP_TITLE"; break;
        case VTERM_PROP_ICONNAME: s = "VTERM_PROP_ICONNAME"; break;
        case VTERM_PROP_REVERSE: s = "VTERM_PROP_REVERSE"; break;
        case VTERM_PROP_CURSORSHAPE: s = "VTERM_PROP_CURSORSHAPE"; break;
        case VTERM_PROP_MOUSE: s = "VTERM_PROP_MOUSE"; break;
        case VTERM_N_PROPS: s = "VTERM_N_PROPS"; break;
    }
    return os << s;
}

} // namespace tvterm

#endif // TVTERM_DEBUG_H
