#include <tvterm/termview.h>
#include <tvterm/termctrl.h>
#include <tvterm/consts.h>

#define Uses_TKeys
#define Uses_TEvent
#include <tvision/tv.h>

namespace tvterm
{

TerminalView::TerminalView( const TRect &bounds, TerminalController &aTermCtrl,
                            const TVTermConstants &aConsts ) noexcept :
    TView(bounds),
    consts(aConsts),
    termCtrl(aTermCtrl)
{
    growMode = gfGrowHiX | gfGrowHiY;
    options |= ofSelectable | ofFirstClick;
    eventMask |= evMouseMove | evMouseAuto | evMouseWheel | evBroadcast;
    showCursor();
}

TerminalView::~TerminalView()
{
    termCtrl.shutDown();
}

void TerminalView::changeBounds(const TRect& bounds)
{
    setBounds(bounds);
    ownerBufferChanged = true;
    drawView();

    TerminalEvent termEvent;
    termEvent.type = TerminalEventType::ViewportResize;
    termEvent.viewportResize = {size.x, size.y};
    termCtrl.sendEvent(termEvent);
}

void TerminalView::setState(ushort aState, bool enable)
{
    if (aState == sfExposed && enable != getState(sfExposed))
        ownerBufferChanged = true;

    TView::setState(aState, enable);

    if (aState == sfFocused)
    {
        TerminalEvent termEvent;
        termEvent.type = TerminalEventType::FocusChange;
        termEvent.focusChange = {enable};
        termCtrl.sendEvent(termEvent);
    }
}

void TerminalView::handleEvent(TEvent &ev)
{
    TView::handleEvent(ev);

    switch (ev.what)
    {
        case evBroadcast:
            if ( ev.message.command == consts.cmCheckTerminalUpdates &&
                 termCtrl.stateHasBeenUpdated() )
                drawView();
            break;

        case evKeyDown:
        {
            TerminalEvent termEvent;
            termEvent.type = TerminalEventType::KeyDown;
            termEvent.keyDown = ev.keyDown;
            termCtrl.sendEvent(termEvent);

            clearEvent(ev);
            break;
        }

        case evMouseDown:
            do {
                handleMouse(ev.what, ev.mouse);
            } while (mouseEvent(ev, evMouse));
            if (ev.what == evMouseUp)
                handleMouse(ev.what, ev.mouse);
            clearEvent(ev);
            break;

        case evMouseWheel:
        case evMouseMove:
        case evMouseAuto:
        case evMouseUp:
            handleMouse(ev.what, ev.mouse);
            clearEvent(ev);
            break;
    }
}

void TerminalView::handleMouse(ushort what, MouseEventType mouse) noexcept
{
    mouse.where = makeLocal(mouse.where);

    TerminalEvent termEvent;
    termEvent.type = TerminalEventType::Mouse;
    termEvent.mouse = {what, mouse};
    termCtrl.sendEvent(termEvent);
}

void TerminalView::draw()
{
    termCtrl.lockState([&] (auto &state) {
        updateCursor(state);
        updateDisplay(state.surface);

        TerminalUpdatedMsg upd {*this, state};
        message(owner, evCommand, consts.cmTerminalUpdated, &upd);
    });
}

void TerminalView::updateCursor(TerminalState &state) noexcept
{
    if (state.cursorChanged)
    {
        state.cursorChanged = false;
        setState(sfCursorVis, state.cursorVisible);
        setState(sfCursorIns, state.cursorBlink);
        setCursor(state.cursorPos.x, state.cursorPos.y);
    }
}

static TerminalSurface::RowDamage rangeToCopy(int y, const TRect &r, const TerminalSurface &surface, bool reuseBuffer)
{
    auto &damage = surface.damageAtRow(y);
    if (reuseBuffer)
        return {
            max(r.a.x, damage.begin),
            min(r.b.x, damage.end),
        };
    else
        return {r.a.x, r.b.x};
}

void TerminalView::updateDisplay(TerminalSurface &surface) noexcept
{
    bool reuseBuffer = canReuseOwnerBuffer();
    TRect r = getExtent().intersect({{0, 0}, surface.size});
    if (0 <= r.a.x && r.a.x < r.b.x && 0 <= r.a.y && r.a.y < r.b.y)
    {
        for (int y = r.a.y; y < r.b.y; ++y)
        {
            auto c = rangeToCopy(y, r, surface, reuseBuffer);
            writeLine(c.begin, y, c.end - c.begin, 1, &surface.at(y, c.begin));
        }
        surface.clearDamage();
        // We don't need to draw the area that is not filled by the surface.
        // It will be blank.
    }
}

bool TerminalView::canReuseOwnerBuffer() noexcept
{
    if (ownerBufferChanged)
    {
        ownerBufferChanged = false;
        return false;
    }
    return owner && owner->buffer;
}

} // namespace tvterm
