#define Uses_TFrame
#define Uses_TEvent
#define Uses_TStaticText
#include <tvision/tv.h>

#include <tvterm/termwnd.h>
#include <tvterm/termview.h>
#include <tvterm/termframe.h>
#include <tvterm/termactiv.h>
#include <tvterm/cmds.h>

TCommandSet TerminalWindow::focusedCmds = []()
{
    TCommandSet ts;
    ts += cmGrabInput;
    ts += cmReleaseInput;
    return ts;
}();

TFrame *TerminalWindow::initFrame(TRect bounds)
{
    return new TerminalFrame(bounds);
}

TerminalWindow::TerminalWindow(const TRect &bounds, TerminalActivity &aTerm) :
    TWindowInit(&TerminalWindow::initFrame),
    TWindow(bounds, nullptr, wnNoNumber)
{
    options |= ofTileable;
    eventMask |= evBroadcast;
    setState(sfShadow, False);
    ((TerminalFrame *) frame)->setTerm(&aTerm);
    view = new TerminalView(getExtent().grow(-1, -1), aTerm);
    insert(view);
}

void TerminalWindow::shutDown()
{
    if (frame)
        ((TerminalFrame *) frame)->setTerm(nullptr);
    view = nullptr;
    TWindow::shutDown();
}

void TerminalWindow::checkChanges() noexcept
{
    if (view && view->term.checkChanges())
    {
        view->drawView();
        bool titleUpdated = view->term.getState([&] (auto &state) {
            return updateTitle(state);
        });
        if (titleUpdated && frame)
            frame->drawView();
    }
}

bool TerminalWindow::updateTitle(TerminalReceivedState &state) noexcept
{
    if (state.titleChanged)
    {
        state.titleChanged = false;
        termTitle = std::move(state.title);
        return true;
    }
    return false;
}

void TerminalWindow::resizeTitle(size_t aCapacity)
{
    if (titleCapacity < aCapacity)
    {
        if (title)
            delete[] title;
        title = new char[aCapacity];
        titleCapacity = aCapacity;
    }
}

const char *TerminalWindow::getTitle(short)
{
    TStringView tail = (helpCtx == hcInputGrabbed)
                            ? " (Input Grab)"
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

void TerminalWindow::handleEvent(TEvent &ev)
{
    bool canClose = false;
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
        case evBroadcast:
            if (ev.message.command == cmIdle)
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
        case evKeyDown:
            canClose = true;
            break;
    }
    if (view && view->term.isClosed() && !(state & sfDragging))
    {
        if (state & sfModal)
            endModal(cmCancel);
        else if (canClose)
        {
            close();
            return;
        }
    }
    TWindow::handleEvent(ev);
}

void TerminalWindow::setState(ushort aState, Boolean enable)
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

ushort TerminalWindow::execute()
{
    auto lastHelpCtx = helpCtx;
    helpCtx = hcInputGrabbed;
    if (frame) frame->drawView();

    TWindow::execute();

    helpCtx = lastHelpCtx;
    if (frame) frame->drawView();
    return 0;
}
