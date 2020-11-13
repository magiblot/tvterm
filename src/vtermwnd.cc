#include <tvterm/vtermwnd.h>
#include <tvterm/vtermview.h>

TVTermWindow::TVTermWindow(const TRect &bounds) :
    TWindowInit(&TVTermWindow::initFrame),
    TWindow(bounds, nullptr, wnNoNumber)
{
    options |= ofTileable;
    setState(sfShadow, False);
    {
        TRect r = getExtent().grow(-1, -1);
        TView *vt = new TVTermView(r, *this);
        insert(vt);
    }
}
