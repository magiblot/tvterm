#define Uses_TKeys
#include <tvision/tv.h>

#include <tvterm/vtermview.h>
#include <tvterm/vtermwnd.h>

TVTermView::TVTermView( const TRect &bounds, TVTermWindow &window,
                        asio::io_context &io ) :
    TView(bounds),
    window(window),
    vterm(*this, io)
{
    growMode = gfGrowHiX | gfGrowHiY;
    options |= ofSelectable | ofFirstClick;
    eventMask |= evMouseMove | evMouseAuto | evMouseWheel | evBroadcast;
    showCursor();
}

void TVTermView::changeBounds(const TRect& bounds)
{
    setBounds(bounds);
    vterm.mDisplay.lock([&] (auto &dState) {
        // The next time we draw() we'll need to copy the whole surface,
        // so we set everything as damaged.
        dState.surface.damageAll();
    });
    vterm.updateChildSize();
    drawView();
}

void TVTermView::handleEvent(TEvent &ev)
{
    TView::handleEvent(ev);
    vterm.handleEvent(ev);
}

void TVTermView::draw()
{
    vterm.mDisplay.lock([&] (auto &dState) {
        if (dState.cursorChanged)
        {
            setState(sfCursorVis, dState.cursorVisible);
            setState(sfCursorIns, dState.cursorBlink);
            setCursor(dState.cursorPos.x, dState.cursorPos.y);
            dState.cursorChanged = false;
        }
        auto &surface = dState.surface;
        TRect r = getExtent().intersect(TRect({0, 0}, surface.size));
        for (int y = r.a.y; y < r.b.y; ++y)
        {
            auto &damage = surface.rowDamage[y];
            int begin = max(r.a.x, damage.begin);
            int end = min(r.b.x, damage.end);
            if (begin < end)
                writeLine(begin, y, end - begin, 1, &surface.at(y, begin));
        }
        surface.clearDamage();
    });
}
