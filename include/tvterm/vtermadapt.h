#ifndef TVTERM_VTERMADAPT_H
#define TVTERM_VTERMADAPT_H

#include <tvterm/ptylisten.h>
#include <tvterm/vterm.h>
#include <tvterm/pty.h>
#include <vector>

struct TEvent;
struct TVTermView;

struct TVTermAdapter
{

    TVTermView &view;
    struct VTerm *vt;
    struct VTermState *state;
    struct VTermScreen *vts;
    PTY pty;
    PTYListener listener;
    bool pending;
    bool resizing;
    bool mouseEnabled;
    std::vector<char> outbuf;

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

#endif // TVTERM_VTERMADAPT_H
