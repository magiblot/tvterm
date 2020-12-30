#define Uses_TFrame
#define Uses_TEvent
#define Uses_TStaticText
#include <tvision/tv.h>

#include <tvterm/vtermwnd.h>
#include <tvterm/vtermview.h>
#include <tvterm/cmds.h>

TCommandSet TVTermWindow::focusedCmds = []()
{
    TCommandSet ts;
    ts += cmGrabInput;
    ts += cmReleaseInput;
    return ts;
}();

TVTermWindow::TVTermWindow(const TRect &bounds) :
    TWindowInit(&TVTermWindow::initFrame),
    TWindow(bounds, nullptr, wnNoNumber)
{
    options |= ofTileable;
    setState(sfShadow, False);
    {
        TRect r = getExtent().grow(-1, -1);
        TView *vt;
        try
        {
            vt = new TVTermView(r, *this);
        }
        catch (std::string err)
        {
            vt = new TStaticText(r, err);
            vt->growMode = gfGrowHiX | gfGrowHiY;
        }
        insert(vt);
    }
}

void TVTermWindow::setTitle(std::string_view text)
{
    std::string_view tail = (helpCtx == hcInputGrabbed)
                          ? " (Input Grab)"
                          : "";
    // For some unknown reason the last character is eaten up by TFrame...
    // We'll find out why later. Let's work around it for the time being.
    char *str = new char[text.size() + tail.size() + 2];
    memcpy(str, text.data(), text.size());
    memcpy(str + text.size(), tail.data(), tail.size());
    str[text.size() + tail.size()] = ' ';
    str[text.size() + tail.size() + 1] = '\0';
    termTitle = {str, text.size()};
    delete[] title; // 'text' could point to 'title', so don't free too early.
    title = str;
    frame->drawView();
}

void TVTermWindow::handleEvent(TEvent &ev)
{
    bool handled = true;
    switch (ev.what)
    {
        case evCommand:
            switch (ev.message.command)
            {
                case cmGrabInput:
                    if (helpCtx != hcInputGrabbed)
                        execute();
                    break;
                case cmIsTerm:
                    ev.message.infoPtr = this;
                    break;
                default: handled = false; break;
            }
            break;
        default: handled = false; break;
    }
    if (handled)
        clearEvent(ev);
    else
        TWindow::handleEvent(ev);
}

void TVTermWindow::setState(ushort aState, Boolean enable)
{
    TWindow::setState(aState, enable);
    if (aState == sfActive)
    {
        if (enable)
            enableCommands(focusedCmds);
        else
            disableCommands(focusedCmds);
    }
}

ushort TVTermWindow::execute()
{
    // Update help context.
    ushort hc = helpCtx;
    helpCtx = hcInputGrabbed;
    // Update title.
    setTitle(termTitle);
    while (true)
    {
        TEvent ev;
        getEvent(ev);
        if ( (ev.what == evCommand && ev.message.command == cmClose) ||
             (ev.what == evMouseDown && !mouseInView(ev.mouse.where)) )
        {
            putEvent(ev);
            break;
        }
        else if (ev.what == evCommand && ev.message.command == cmReleaseInput)
            break;
        else
            handleEvent(ev);
    }
    // Restore help context.
    helpCtx = hc;
    // Restore title.
    setTitle(termTitle);
    return 0;
}
