#ifndef TVTERM_VTERMADAPT_H
#define TVTERM_VTERMADAPT_H

#include <tvision/tv.h>

#include <tvterm/ptylisten.h>
#include <tvterm/vterm.h>
#include <tvterm/util.h>
#include <tvterm/pty.h>
#include <utility>
#include <vector>
#include <memory>

struct TEvent;
struct TVTermView;

struct TVTermAdapter
{

    struct LineStack
    {
        std::vector<std::pair<std::unique_ptr<const VTermScreenCell[]>, size_t>> stack;
        void push(size_t cols, const VTermScreenCell *cells);
        bool pop(const TVTermAdapter &vterm, size_t cols, VTermScreenCell *cells);
        TSpan<const VTermScreenCell> top() const;
    };

    TVTermView &view;
    struct VTerm *vt;
    struct VTermState *state;
    struct VTermScreen *vts;
    managed_lifetime<PTY> pty;
    managed_lifetime<PTYListener> listener;
    bool pending;
    bool resizing;
    bool mouseEnabled;
    bool altScreenEnabled;
    std::vector<char> outbuf;
    LineStack linestack;

    static const VTermScreenCallbacks callbacks;

    TVTermAdapter(TVTermView &);
    ~TVTermAdapter();

    void read();
    void handleEvent(TEvent &ev);
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
{
    VTermScreenCell cell {};
    cell.width = 1;
    vterm_state_get_default_colors(state, &cell.fg, &cell.bg);
    return cell;
}

inline TSpan<const VTermScreenCell> TVTermAdapter::LineStack::top() const
{
    auto &pair = stack.back();
    return {pair.first.get(), pair.second};
}

#endif // TVTERM_VTERMADAPT_H
