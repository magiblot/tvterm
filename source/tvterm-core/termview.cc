#include <tvterm/termview.h>
#include <tvterm/termctrl.h>
#include <tvterm/consts.h>

#define Uses_TKeys
#define Uses_TEvent
#include <tvision/tv.h>

#include <stdio.h>

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
    clearSelection();
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
        case evCommand:
            if (ev.message.command == consts.cmCopySelection)
            {
                copySelection();
                clearEvent(ev);
            }
            break;

        case evBroadcast:
            if ( ev.message.command == consts.cmCheckTerminalUpdates &&
                 termCtrl.stateHasBeenUpdated() )
                drawView();
            break;

        case evKeyDown:
        {
            // Clear selection on any keypress
            if (selection.active || selection.valid)
            {
                clearSelection();
                ownerBufferChanged = true;
                drawView();
            }

            TerminalEvent termEvent;
            termEvent.type = TerminalEventType::KeyDown;
            termEvent.keyDown = ev.keyDown;
            termCtrl.sendEvent(termEvent);

            clearEvent(ev);
            break;
        }

        case evMouseDown:
            // Left button + !mouseEnabled → selection mode
            if (!cachedMouseEnabled && (ev.mouse.buttons & mbLeftButton))
            {
                TPoint localPos = makeLocal(ev.mouse.where);
                handleSelectionMouse(evMouseDown, localPos);
                while (mouseEvent(ev, evMouse))
                {
                    localPos = makeLocal(ev.mouse.where);
                    handleSelectionMouse(ev.what, localPos);
                }
                if (ev.what == evMouseUp)
                {
                    localPos = makeLocal(ev.mouse.where);
                    handleSelectionMouse(evMouseUp, localPos);
                }
                clearEvent(ev);
            }
            else
            {
                // Forward to emulator (existing behavior)
                do {
                    handleMouse(ev.what, ev.mouse);
                } while (mouseEvent(ev, evMouse));
                if (ev.what == evMouseUp)
                    handleMouse(ev.what, ev.mouse);
                clearEvent(ev);
            }
            break;

        case evMouseWheel:
            // Always forward wheel to emulator
            handleMouse(ev.what, ev.mouse);
            clearEvent(ev);
            break;

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

void TerminalView::handleSelectionMouse(ushort what, TPoint localPos) noexcept
{
    // Clamp to view bounds
    localPos.x = max(0, min(localPos.x, size.x - 1));
    localPos.y = max(0, min(localPos.y, size.y - 1));

    if (what & evMouseDown)
    {
        selection.anchor = localPos;
        selection.current = localPos;
        selection.active = true;
        selection.valid = false;
        ownerBufferChanged = true;
        drawView();
    }
    else if (what & (evMouseMove | evMouseAuto))
    {
        if (selection.active && localPos != selection.current)
        {
            selection.current = localPos;
            ownerBufferChanged = true;
            drawView();
        }
    }
    else if (what & evMouseUp)
    {
        if (selection.active)
        {
            selection.current = localPos;
            selection.active = false;
            selection.valid = true;
            ownerBufferChanged = true;
            drawView();
        }
    }
}

void TerminalView::normalizeSelection(TPoint &start, TPoint &end) const noexcept
{
    TPoint a = selection.anchor;
    TPoint b = selection.current;
    if (a.y < b.y || (a.y == b.y && a.x <= b.x))
    {
        start = a;
        end = b;
    }
    else
    {
        start = b;
        end = a;
    }
}

void TerminalView::clearSelection() noexcept
{
    selection.active = false;
    selection.valid = false;
}

void TerminalView::draw()
{
    termCtrl.lockState([&] (auto &state) {
        cachedMouseEnabled = state.mouseEnabled;
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
            if (c.begin < c.end)
                writeLine(c.begin, y, c.end - c.begin, 1, &surface.at(y, c.begin));
        }
        surface.clearDamage();
    }

    // Apply selection highlight
    if (selection.active || selection.valid)
        applySelectionHighlight(surface);
}

void TerminalView::applySelectionHighlight(TerminalSurface &surface) noexcept
{
    TPoint start, end;
    normalizeSelection(start, end);

    // Clamp against surface and view bounds
    int limitX = min(surface.size.x, size.x);
    int limitY = min(surface.size.y, size.y);
    if (limitX <= 0 || limitY <= 0)
        return;
    start.x = max(0, min(start.x, limitX - 1));
    start.y = max(0, min(start.y, limitY - 1));
    end.x = max(0, min(end.x, limitX - 1));
    end.y = max(0, min(end.y, limitY - 1));

    for (int y = start.y; y <= end.y; ++y)
    {
        int selBegin = (y == start.y) ? start.x : 0;
        int selEnd = (y == end.y) ? end.x + 1 : limitX;
        selBegin = max(selBegin, 0);
        selEnd = min(selEnd, limitX);

        if (selBegin >= selEnd)
            continue;

        // Adjust for wide char boundaries
        if (selBegin > 0 && surface.at(y, selBegin)._ch.isWideCharTrail())
            --selBegin;
        if (selEnd < limitX && surface.at(y, selEnd - 1).isWide())
            ++selEnd;
        selEnd = min(selEnd, limitX);

        int count = selEnd - selBegin;
        if (count <= 0)
            continue;

        // Build modified cells with reversed attributes
        TScreenCell buf[256];
        TScreenCell *cells = (count <= 256) ? buf : new TScreenCell[count];

        for (int i = 0; i < count; ++i)
        {
            cells[i] = surface.at(y, selBegin + i);
            ::setAttr(cells[i], ::reverseAttribute(::getAttr(cells[i])));
        }

        writeLine(selBegin, y, count, 1, cells);

        if (cells != buf)
            delete[] cells;
    }
}

void TerminalView::copySelection() noexcept
{
    if (!selection.valid)
        return;

    GrowArray text;
    termCtrl.lockState([&] (auto &state) {
        auto &surface = state.surface;
        TPoint start, end;
        normalizeSelection(start, end);

        int limitX = surface.size.x;
        int limitY = surface.size.y;
        for (int y = start.y; y <= end.y && y < limitY; ++y)
        {
            int lineBegin = (y == start.y) ? start.x : 0;
            int lineEnd = (y == end.y) ? end.x + 1 : limitX;
            lineBegin = max(lineBegin, 0);
            lineEnd = min(lineEnd, limitX);

            for (int x = lineBegin; x < lineEnd; ++x)
            {
                auto &cell = surface.at(y, x);
                if (cell._ch.isWideCharTrail())
                    continue;

                TStringView cellText = cell._ch.getText();
                if (cellText.size() == 1 && cellText[0] == '\0')
                {
                    char sp = ' ';
                    text.push(&sp, 1);
                }
                else
                {
                    text.push(cellText.data(), cellText.size());
                }
            }

            if (y < end.y)
            {
                char nl = '\n';
                text.push(&nl, 1);
            }
        }
    });

    if (text.size() > 0)
    {
#if !defined(_WIN32)
        FILE *pipe = popen(
            "pbcopy 2>/dev/null || "
            "xclip -selection clipboard 2>/dev/null || "
            "xsel --clipboard --input 2>/dev/null",
            "w"
        );
        if (pipe)
        {
            fwrite(text.data(), 1, text.size(), pipe);
            pclose(pipe);
        }
#else
        // TODO: Windows clipboard via OpenClipboard/SetClipboardData
#endif
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
