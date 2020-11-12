#include <tvterm/vtermview.h>

VTermView::VTermView(const TRect &bounds) :
    TView(bounds)
{
    growMode = gfGrowHiX | gfGrowHiY;
    options |= ofSelectable | ofFirstClick;
    showCursor();
}
