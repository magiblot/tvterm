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
    else if ( ev.what == evKeyDown && isDisconnected() &&
              !(state & (sfDragging | sfModal)) )
    {
        close();
        return;
    }
    super::handleEvent(ev);
}
