#define Uses_TFrame
#define Uses_TEvent
#define Uses_TStaticText
#include <tvision/tv.h>

#include <tvterm/termwnd.h>
#include <tvterm/termview.h>
#include <tvterm/termframe.h>
#include <tvterm/termctrl.h>
#include <tvterm/consts.h>

namespace tvterm
{

TFrame *BasicTerminalWindow::initFrame(TRect bounds)
{
    return new BasicTerminalFrame(bounds);
}

BasicTerminalWindow::BasicTerminalWindow( const TRect &bounds,
                                          TerminalController &termCtrl,
                                          const TVTermConstants &aConsts
                                        ) noexcept :
    TWindowInit(&BasicTerminalWindow::initFrame),
    TWindow(bounds, nullptr, wnNoNumber),
    consts(aConsts)
{
    options |= ofTileable;
    eventMask |= evBroadcast;
    setState(sfShadow, False);
    view = new TerminalView(getExtent().grow(-1, -1), termCtrl, aConsts);
    insert(view);
}

void BasicTerminalWindow::shutDown()
{
    view = nullptr;
    TWindow::shutDown();
}

void BasicTerminalWindow::checkChanges(TerminalUpdatedMsg &upd) noexcept
{
    if (frame && updateTitle(upd.view.termCtrl, upd.state))
        frame->drawView();
}

bool BasicTerminalWindow::updateTitle( TerminalController &term,
                                       TerminalState &state ) noexcept
{
    if (state.titleChanged)
    {
        state.titleChanged = false;
        termTitle = std::move(state.title);
        return true;
    }
    // When the terminal is closed for the first time, 'state.title' does not
    // change but we still need to redraw the title.
    return term.clientIsDisconnected();
}

void BasicTerminalWindow::resizeTitle(size_t aCapacity)
{
    if (titleCapacity < aCapacity)
    {
        if (title)
            delete[] title;
        title = new char[aCapacity];
        titleCapacity = aCapacity;
    }
}

bool BasicTerminalWindow::isDisconnected() const noexcept
{
    return !view || view->termCtrl.clientIsDisconnected();
}

const char *BasicTerminalWindow::getTitle(short)
{
    TStringView tail = isDisconnected()                 ? " (Disconnected)"
                     : helpCtx == consts.hcInputGrabbed ? " (Input Grab)"
                                                        : "";
    TStringView text = {termTitle.data(), termTitle.size()};
    if (size_t length = text.size() + tail.size())
    {
        resizeTitle(length + 1);
        auto *title = (char *) this->title;
        memcpy(title, text.data(), text.size());
        memcpy(title + text.size(), tail.data(), tail.size());
        title[length] = '\0';
        return title;
    }
    return nullptr;
}

void BasicTerminalWindow::handleEvent(TEvent &ev)
{
    switch (ev.what)
    {
        case evCommand:
            if ( ev.message.command == consts.cmGrabInput &&
                 helpCtx != consts.hcInputGrabbed && owner )
            {
                owner->execView(this);
                clearEvent(ev);
            }
            else if ( (ev.message.command == cmClose ||
                       ev.message.command == consts.cmReleaseInput) &&
                      (state & sfModal) )
            {
                endModal(cmCancel);
                if (ev.message.command == cmClose)
                    putEvent(ev);
                clearEvent(ev);
            }
            else if ( ev.message.command == consts.cmTerminalUpdated &&
                      ev.message.infoPtr )
                checkChanges(*(TerminalUpdatedMsg *) ev.message.infoPtr);
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
    if (isDisconnected() && (state & sfModal))
        endModal(cmCancel);
    TWindow::handleEvent(ev);
}

void BasicTerminalWindow::setState(ushort aState, Boolean enable)
{
    TWindow::setState(aState, enable);
    if (aState == sfActive)
    {
        for (ushort cmd : consts.focusedCmds())
            if (enable)
                enableCommand(cmd);
            else
                disableCommand(cmd);
    }
}

ushort BasicTerminalWindow::execute()
{
    auto lastHelpCtx = helpCtx;
    helpCtx = consts.hcInputGrabbed;
    if (frame) frame->drawView();

    TWindow::execute();

    helpCtx = lastHelpCtx;
    if (frame) frame->drawView();
    return 0;
}

} // namespace tvterm
