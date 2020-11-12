#include <tvterm/vtermwnd.h>
#include <tvterm/vtermview.h>

VTermWindow::VTermWindow(const TRect &bounds) :
    TWindowInit(&VTermWindow::initFrame),
    TWindow(bounds, nullptr, wnNoNumber)
{
    options |= ofTileable;
    setState(sfShadow, False);
    {
        TRect r = getExtent().grow(-1, -1);
        TView *vt = new VTermView(r);
        insert(vt);
    }
}
