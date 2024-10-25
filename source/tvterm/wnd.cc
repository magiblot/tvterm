#include "wnd.h"
#include "cmds.h"

#define Uses_TEvent
#include <tvision/tv.h>

const tvterm::TVTermConstants TerminalWindow::appConsts =
{
    cmCheckTerminalUpdates,
    cmTerminalUpdated,
    cmGrabInput,
    cmReleaseInput,
    hcInputGrabbed,
};

void TerminalWindow::handleEvent(TEvent &ev)
{
    if ( ev.what == evBroadcast &&
         ev.message.command == cmGetOpenTerms && !isDisconnected() )
        *(size_t *) ev.message.infoPtr += 1;
    else if( ev.what == evCommand && ev.message.command == cmZoom &&
             (!ev.message.infoPtr || ev.message.infoPtr == this) )
    {
        zoom();
        clearEvent(ev);
    }
    else if ( ev.what == evKeyDown && isDisconnected() &&
              !(state & (sfDragging | sfModal)) )
    {
        close();
        return;
    }
    Super::handleEvent(ev);
}

void TerminalWindow::sizeLimits(TPoint &min, TPoint &max)
{
    Super::sizeLimits(min, max);
    if (owner)
    {
        max = owner->size;
        max.x += 2;
        max.y += 1;
    }
}

void TerminalWindow::zoom() noexcept
{
    TPoint minSize, maxSize;
    sizeLimits(minSize, maxSize);
    if (size != maxSize)
    {
        zoomRect = getBounds();

        // A maximized terminal shows just the title bar and cannot be dragged.
        TRect r(0, 0, maxSize.x, maxSize.y);
        r.move(-1, 0);
        growMode = gfGrowHiX | gfGrowHiY;
        flags &= ~(wfMove | wfGrow);

        locate(r);
    }
    else
    {
        growMode = gfGrowAll | gfGrowRel;
        flags |= wfMove | wfGrow;

        locate(zoomRect);
    }
}
