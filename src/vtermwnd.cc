#define Uses_TFrame
#define Uses_TEvent
#define Uses_TStaticText
#include <tvision/tv.h>

#include <tvterm/vtermwnd.h>
#include <tvterm/vtermview.h>
#include <tvterm/vtermframe.h>
#include <tvterm/cmds.h>

TCommandSet TVTermWindow::focusedCmds = []()
{
    TCommandSet ts;
    ts += cmGrabInput;
    ts += cmReleaseInput;
    return ts;
}();

TFrame *TVTermWindow::initFrame(TRect bounds)
{
    return new TVTermFrame(bounds);
}

TVTermWindow::TVTermWindow(const TRect &bounds, asio::io_context &io) :
    TWindowInit(&TVTermWindow::initFrame),
    TWindow(bounds, nullptr, wnNoNumber),
    view(nullptr)
{
    options |= ofTileable;
    eventMask |= evBroadcast;
    setState(sfShadow, False);
    {
        TRect r = getExtent().grow(-1, -1);
        TView *v;
        try
        {
            v = view = new TVTermView(r, *this, io);
        }
        catch (std::string err)
        {
            v = new TStaticText(r, err);
            v->growMode = gfGrowHiX | gfGrowHiY;
        }
        insert(v);
    }
    ((TVTermFrame *) frame)->setTerm(view);
}

void TVTermWindow::shutDown()
{
    view = nullptr;
    TWindow::shutDown();
}

void TVTermWindow::setTitle(std::string_view text)
{
    std::string_view tail = (helpCtx == hcInputGrabbed)
                          ? " (Input Grab)"
                          : "";
    char *str = new char[text.size() + tail.size() + 1];
    memcpy(str, text.data(), text.size());
    memcpy(str + text.size(), tail.data(), tail.size());
    str[text.size() + tail.size()] = '\0';
    termTitle = {str, text.size()};
    delete[] title; // 'text' could point to 'title', so don't free too early.
    title = str;
    frame->drawView();
}

bool TVTermWindow::ptyClosed() const
{
    return view && view->vterm.pty.closed();
}

void TVTermWindow::handleEvent(TEvent &ev)
{
    switch (ev.what)
    {
        case evCommand:
            switch (ev.message.command)
            {
                case cmGrabInput:
                    if (helpCtx != hcInputGrabbed && owner)
                    {
                        owner->execView(this);
                        clearEvent(ev);
                    }
                    break;
                case cmIsTerm:
                    ev.message.infoPtr = this;
                    clearEvent(ev);
                    break;
                case cmClose:
                case cmReleaseInput:
                    if (state & sfModal)
                    {
                        endModal(cmCancel);
                        if (ev.message.command == cmClose)
                            putEvent(ev);
                        clearEvent(ev);
                    }
                    break;
            }
            break;
        case evMouseDown:
            if ((state & sfModal) && !mouseInView(ev.mouse.where))
            {
                endModal(cmCancel);
                putEvent(ev);
                clearEvent(ev);
            }
            break;
    }
    if (ptyClosed() && !(state & sfDragging))
    {
        if (state & sfModal)
            endModal(cmCancel);
        else
        {
            close();
            return;
        }
    }
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
    TWindow::execute();
    // Restore help context.
    helpCtx = hc;
    // Restore title.
    setTitle(termTitle);
    return 0;
}
