#define Uses_TFrame
#include <tvision/tv.h>

#include <tvterm/vtermwnd.h>
#include <tvterm/vtermview.h>
#include <tvterm/cmds.h>

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
    if (ev.what == evCommand && ev.message.command == cmGrabInput)
    {
        if (helpCtx != hcInputGrabbed)
            execute();
        clearEvent(ev);
    }
    else
        TWindow::handleEvent(ev);
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
