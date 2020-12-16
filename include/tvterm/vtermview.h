#ifndef TVTERM_VTERMVIEW_H
#define TVTERM_VTERMVIEW_H

#define Uses_TView
#define Uses_TGroup
#include <tvision/tv.h>

#include <tvterm/vtermadapt.h>

struct TVTermWindow;

struct TVTermView : public TView
{

    TVTermWindow &window;
    TVTermAdapter vterm;

    TVTermView(const TRect &bounds, TVTermWindow &window);

    TScreenCell& at(int y, int x);
    void changeBounds(const TRect& bounds) override;
    void handleEvent(TEvent &ev) override;

};

inline TScreenCell& TVTermView::at(int y, int x)
{
    // Temporary solution: draw directly on the owner's buffer.
    TRect r = getBounds();
    return owner->buffer[owner->size.x * (r.a.y + y) + (r.a.x + x)];
}

#endif // TVTERM_VTERMVIEW_H
