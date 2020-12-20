#define Uses_TText
#define Uses_TKeys
#include <tvision/tv.h>

#include <tvterm/vtermview.h>
#include <tvterm/vtermwnd.h>

TVTermView::TVTermView(const TRect &bounds, TVTermWindow &window) :
    TView(bounds),
    window(window),
    vterm(*this)
{
    growMode = gfGrowHiX | gfGrowHiY;
    options |= ofSelectable | ofFirstClick;
    eventMask |= evMouseMove | evMouseAuto | evMouseWheel;
    showCursor();
}

void TVTermView::changeBounds(const TRect& bounds)
{
    TView::changeBounds(bounds);
    vterm.updateChildSize();
}

void TVTermView::handleEvent(TEvent &ev)
{
    TView::handleEvent(ev);
    vterm.handleEvent(ev);
}

void TVTermView::draw()
{
    // TView::draw will be necessary until TVTermAdapter::damage becomes capable
    // of filling cell attributes.
    TView::draw();
    vterm.damageAll();
}
