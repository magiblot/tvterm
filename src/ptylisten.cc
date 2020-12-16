#define Uses_TEvent
#include <tvision/tv.h>

#include <tvterm/ptylisten.h>
#include <tvterm/vtermadapt.h>
#include <tvterm/cmds.h>

bool PTYListener::getEvent(TEvent &ev)
{
    if (!vterm.pending)
    {
        vterm.pending = true;
        ev.what = evCommand;
        ev.message.command = cmVTermReadable;
        ev.message.infoPtr = &vterm;
        return true;
    }
    return false;
}
