#ifndef TVTERM_VTERMVIEW_H
#define TVTERM_VTERMVIEW_H

#define Uses_TView
#include <tvision/tv.h>

#include <sys/types.h>
#include <vector>

struct TVTermWindow;
struct TVTermView;
struct PTYListener;

struct TVTermAdapter
{

    TVTermView &view;
    struct VTerm *vt;
    struct VTermState *state;
    struct VTermScreen *vts;
    pid_t child_pid;
    int master_fd;
    PTYListener *listener;
    bool pending;
    bool resizing;
    bool mouseEnabled;
    std::vector<char> outbuf;

    static const VTermScreenCallbacks callbacks;

    TVTermAdapter(TVTermView &);
    ~TVTermAdapter();

    void initTermios(struct termios &, struct winsize &) const;

    void read();
    void setSize(int rows, int cols);
    void handleEvent(TEvent &ev);
    void flushOutput();

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

struct TVTermView : public TView
{

    TVTermWindow &window;
    TVTermAdapter vterm;

    TVTermView(const TRect &bounds, TVTermWindow &window);

    TScreenCell& at(int y, int x);
    void changeBounds(const TRect& bounds) override;
    void handleEvent(TEvent &ev) override;

};

#endif // TVTERM_VTERMVIEW_H
