#ifndef TVTERM_VTERMVIEW_H
#define TVTERM_VTERMVIEW_H

#define Uses_TView
#define Uses_TGroup
#include <tvision/tv.h>

#include <tvterm/vtermadapt.h>
#include <tvterm/io.h>

struct TVTermWindow;

struct TVTermView : public TView
{

    TVTermWindow &window;
    TVTermAdapter vterm;

    TVTermView(const TRect &bounds, TVTermWindow &window, asio::io_context &io);

    void changeBounds(const TRect& bounds) override;
    void handleEvent(TEvent &ev) override;
    void draw() override;

};

#endif // TVTERM_VTERMVIEW_H
