#ifndef TVTERM_VTERMVIEW_H
#define TVTERM_VTERMVIEW_H

#define Uses_TView
#include <tvision/tv.h>

struct TVTermWindow;
struct TVTermView;

struct TVTermAdapter
{

    TVTermView &view;
    struct VTerm *vt;
    struct VTermScreen *vts;

    static const VTermScreenCallbacks callbacks;

    TVTermAdapter(TVTermView &);
    ~TVTermAdapter();

    static int damage(VTermRect rect, void *user);
    static int moverect(VTermRect dest, VTermRect src, void *user);
    static int movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user);
    static int settermprop(VTermProp prop, VTermValue *val, void *user);
    static int bell(void *user);
    static int resize(int rows, int cols, void *user);
    static int sb_pushline(int cols, const VTermScreenCell *cells, void *user);
    static int sb_popline(int cols, VTermScreenCell *cells, void *user);

};

struct TVTermView : public TView
{

    TVTermWindow &window;
    TVTermAdapter vterm;

    TVTermView(const TRect &bounds, TVTermWindow &window);

};

#endif // TVTERM_VTERMVIEW_H
