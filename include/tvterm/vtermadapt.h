#ifndef TVTERM_VTERMADAPT_H
#define TVTERM_VTERMADAPT_H

#define Uses_TDrawSurface
#include <tvision/tv.h>

#include <tvterm/ptylisten.h>
#include <tvterm/vterm.h>
#include <tvterm/util.h>
#include <tvterm/pty.h>
#include <tvterm/io.h>
#include <utility>
#include <vector>
#include <memory>
#include <chrono>

#include <tvision/internal/mutex.h>

struct TEvent;
struct TVTermView;

struct TVTermSurface : protected TDrawSurface
{
    struct Range
    {
        int begin, end;
    };

    std::vector<Range> rowDamage;

    TVTermSurface() = default;
    void resize(TPoint aSize);
    void clearDamage();
    void damageAll();
    using TDrawSurface::at;
    using TDrawSurface::size;

};

inline void TVTermSurface::resize(TPoint aSize)
{
    TDrawSurface::resize(aSize);
    clearDamage();
}

inline void TVTermSurface::clearDamage()
{
    rowDamage.resize(0);
    rowDamage.resize(max(0, size.y), {INT_MAX, INT_MIN});
}

inline void TVTermSurface::damageAll()
{
    rowDamage.resize(0);
    rowDamage.resize(max(0, size.y), {INT_MIN, INT_MAX});
}

enum : ushort
{
    vtClosed        = 0x0001,
    vtUpdated       = 0x0002,
    vtTitleSet      = 0x0004,
};

struct TVTermAdapter
{

    struct LineStack
    {
        enum { maxSize = 10000 };

        std::vector<std::pair<std::unique_ptr<const VTermScreenCell[]>, size_t>> stack;
        void push(size_t cols, const VTermScreenCell *cells);
        bool pop(const TVTermAdapter &vterm, size_t cols, VTermScreenCell *cells);
        TSpan<const VTermScreenCell> top() const;
    };

    struct VTState
    {
        struct VTerm *vt;
        struct VTermState *state;
        struct VTermScreen *vts;
        std::vector<char> writeBuf;
        std::vector<char> strFragBuf;
        LineStack linestack;
        bool mouseEnabled {false};
        bool altScreenEnabled {false};
    };

    struct DisplayState
    {
        TVTermSurface surface;
        bool cursorChanged {false};
        TPoint cursorPos {0, 0};
        bool cursorVisible {false};
        bool cursorBlink {false};
    };

    TVTermView &view;
    PTY pty;
    PTYListener listener;
    asio::io_context::strand strand;
    TMutex<VTState> mVT;
    TMutex<DisplayState> mDisplay;
    ushort state;

    static const VTermScreenCallbacks callbacks;

    static void childActions();

    TVTermAdapter(TVTermView &, asio::io_context &);
    ~TVTermAdapter();

    template <class Func>
    void sendVT(Func &&);

    void readInput(TSpan<const char> buf);
    void flushDamage();
    void handleEvent(TEvent &ev);
    void idle();
    void flushOutput();

    void updateChildSize();
    void updateParentSize();
    TPoint getChildSize() const;
    void setChildSize(TPoint s) const;
    void setParentSize(TPoint s);

    void damageAll();

    VTermScreenCell getDefaultCell() const;

    void writeOutput(const char *data, size_t size);
    int damage(VTermRect rect);
    int moverect(VTermRect dest, VTermRect src);
    int movecursor(VTermPos pos, VTermPos oldpos, int visible);
    int settermprop(VTermProp prop, VTermValue *val);
    int bell();
    int resize(int rows, int cols);
    int sb_pushline(int cols, const VTermScreenCell *cells);
    int sb_popline(int cols, VTermScreenCell *cells);

};

inline VTermScreenCell TVTermAdapter::getDefaultCell() const
// Pre: mVT is locked.
{
    VTermScreenCell cell {};
    cell.width = 1;
    vterm_state_get_default_colors(mVT.get().state, &cell.fg, &cell.bg);
    return cell;
}

inline TSpan<const VTermScreenCell> TVTermAdapter::LineStack::top() const
{
    auto &pair = stack.back();
    return {pair.first.get(), pair.second};
}

#endif // TVTERM_VTERMADAPT_H
