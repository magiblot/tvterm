#include <tvterm/cmds.h>
#include <tvterm/termview.h>
#include <tvterm/termactiv.h>
#include <tvterm/debug.h>

#define Uses_TKeys
#define Uses_TEvent
#include <tvision/tv.h>

TerminalView::TerminalView(const TRect &bounds, TerminalActivity &aTerm) noexcept :
    TView(bounds),
    term(aTerm)
{
    growMode = gfGrowHiX | gfGrowHiY;
    options |= ofSelectable | ofFirstClick;
    eventMask |= evMouseMove | evMouseAuto | evMouseWheel | evBroadcast;
    showCursor();
}

TerminalView::~TerminalView()
{
    delete &term;
}

void TerminalView::changeBounds(const TRect& bounds)
{
    setBounds(bounds);
    term.getState([&] (auto &state) {
        // If by the time 'draw()' gets called the surface still hasn't been
        // redrawn, at least copy the whole surface.
        state.surface.damageAll();
    });
    term.changeSize(size);
    drawView();
}

void TerminalView::handleEvent(TEvent &ev)
{
    TView::handleEvent(ev);
    bool handled = true;
    switch (ev.what)
    {
        case evKeyDown:
            term.handleKeyDown(ev.keyDown);
            break;
        case evMouseDown:
            do {
                handleMouse(ev.what, ev.mouse);
            } while (mouseEvent(ev, evMouse));
            if (ev.what == evMouseUp)
                handleMouse(ev.what, ev.mouse);
            break;
        case evMouseWheel:
        case evMouseMove:
        case evMouseAuto:
        case evMouseUp:
            handleMouse(ev.what, ev.mouse);
            break;
        default:
            handled = false;
            break;
    }
    if (handled && ev.what != evBroadcast)
        clearEvent(ev);
}

void TerminalView::handleMouse(ushort what, MouseEventType mouse) noexcept
{
    mouse.where = makeLocal(mouse.where);
    term.handleMouse(what, mouse);
}

void TerminalView::draw()
{
    term.getState([&] (auto &state) {
        updateCursor(state);
        updateDisplay(state.surface);
    });
}

void TerminalView::updateCursor(TerminalReceivedState &state) noexcept
{
    if (state.cursorChanged)
    {
        state.cursorChanged = false;
        setState(sfCursorVis, state.cursorVisible);
        setState(sfCursorIns, state.cursorBlink);
        setCursor(state.cursorPos.x, state.cursorPos.y);
    }
}

void TerminalView::updateDisplay(TerminalSurface &surface) noexcept
{
    TRect r = getExtent().intersect({{0, 0}, surface.size});
    if (0 <= r.a.x && r.a.x < r.b.x && 0 <= r.a.y && r.a.y < r.b.y)
    {
        for (int y = r.a.y; y < r.b.y; ++y)
        {
            auto &damage = surface.damageAt(y);
            int begin = max(r.a.x, damage.begin);
            int end = min(r.b.x, damage.end);
            if (begin <= damage.begin)
                damage.begin = max(damage.begin, end);
            if (damage.end <= end)
                damage.end = min(damage.end, begin);
            writeLine(begin, y, end - begin, 1, &surface.at(y, begin));

        }
        // We don't need to draw the area that is not filled by the surface.
        // It will be blank or will still contain the surface's previous contents.
    }
    else
        TView::draw();
}
