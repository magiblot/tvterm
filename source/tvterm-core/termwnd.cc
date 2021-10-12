#define Uses_TFrame
#define Uses_TEvent
#define Uses_TStaticText
#include <tvision/tv.h>

#include <tvterm/termwnd.h>
#include <tvterm/termview.h>
#include <tvterm/termframe.h>
#include <tvterm/termactiv.h>

namespace tvterm
{

TFrame *BasicTerminalWindow::initFrame(TRect bounds)
{
    return new BasicTerminalFrame(bounds);
}

BasicTerminalWindow::BasicTerminalWindow( const TRect &bounds,
                                          TerminalActivity &aTerm,
                                          const BasicTerminalWindowAppConstants &aAppConsts
                                        ) noexcept :
    TWindowInit(&BasicTerminalWindow::initFrame),
    TWindow(bounds, nullptr, wnNoNumber),
    appConsts(aAppConsts)
{
    options |= ofTileable;
    eventMask |= evBroadcast;
    setState(sfShadow, False);
    ((BasicTerminalFrame *) frame)->setTerm(&aTerm);
    view = new TerminalView(getExtent().grow(-1, -1), aTerm);
    insert(view);
}

void BasicTerminalWindow::shutDown()
{
    if (frame)
        ((BasicTerminalFrame *) frame)->setTerm(nullptr);
    view = nullptr;
    TWindow::shutDown();
}

void BasicTerminalWindow::checkChanges() noexcept
{
    bool frameChanged = false;
    if (view)
    {
        auto &term = view->term;
        if (term.checkChanges())
        {
            view->drawView();
            frameChanged |= term.getState([&] (auto &state) {
                return updateTitle(term, state);
            });
        }
        auto termSize = term.getSize();
        frameChanged |= lastTermSize != termSize;
        lastTermSize = termSize;
        if (frame && frameChanged)
            frame->drawView();
    }
}

bool BasicTerminalWindow::updateTitle( TerminalActivity &term,
                                  TerminalSharedState &state ) noexcept
{
    if (state.titleChanged)
    {
        state.titleChanged = false;
        termTitle = std::move(state.title);
        return true;
    }
    // When the terminal is closed for the first time, 'state.title' does not
    // change but we still need to redraw the title.
    return term.isClosed();
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

bool BasicTerminalWindow::isClosed() const noexcept
{
    return !view || view->term.isClosed();
}

const char *BasicTerminalWindow::getTitle(short)
{
    TStringView tail = isClosed()                          ? " (Disconnected)"
                     : helpCtx == appConsts.hcInputGrabbed ? " (Input Grab)"
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
            if ( ev.message.command == appConsts.cmGrabInput &&
                 helpCtx != appConsts.hcInputGrabbed && owner )
            {
                owner->execView(this);
                clearEvent(ev);
            }
            else if ( (ev.message.command == cmClose ||
                       ev.message.command == appConsts.cmReleaseInput) &&
                      (state & sfModal) )
            {
                endModal(cmCancel);
                if (ev.message.command == cmClose)
                    putEvent(ev);
                clearEvent(ev);
            }
            break;
        case evBroadcast:
            if (ev.message.command == appConsts.cmIdle)
                checkChanges();
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
    if (isClosed() && (state & sfModal))
        endModal(cmCancel);
    TWindow::handleEvent(ev);
}

void BasicTerminalWindow::setState(ushort aState, Boolean enable)
{
    TWindow::setState(aState, enable);
    if (aState == sfActive)
    {
        for (ushort cmd : appConsts.focusedCmds())
            if (enable)
                enableCommand(cmd);
            else
                disableCommand(cmd);
        if (view)
            view->term.sendFocus(enable);
    }
}

ushort BasicTerminalWindow::execute()
{
    auto lastHelpCtx = helpCtx;
    helpCtx = appConsts.hcInputGrabbed;
    if (frame) frame->drawView();

    TWindow::execute();

    helpCtx = lastHelpCtx;
    if (frame) frame->drawView();
    return 0;
}

} // namespace tvterm
