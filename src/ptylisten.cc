#include <tvterm/ptylisten.h>
#include <tvterm/vtermview.h>

PTYListener::PTYListener(TVTermAdapter &vterm) :
    vterm(vterm)
{
    addListener(this, vterm.master_fd);
}

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
