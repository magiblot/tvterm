#ifndef TVTERM_TERMVIEW_H
#define TVTERM_TERMVIEW_H

#define Uses_TView
#define Uses_TGroup
#include <tvision/tv.h>


struct MouseEventType;

class TerminalActivity;
class TerminalSurface;
struct TerminalReceivedState;

class TerminalView : public TView
{
    bool ownerBufferChanged {false};

    void handleMouse(ushort what, MouseEventType mouse) noexcept;
    void updateCursor(TerminalReceivedState &state) noexcept;
    void updateDisplay(TerminalSurface &surface) noexcept;
    bool canReuseOwnerBuffer() noexcept;

public:

    TerminalActivity &term;

    // Takes ownership over 'aTerm'.
    TerminalView(const TRect &bounds, TerminalActivity &aTerm) noexcept;
    ~TerminalView();

    void changeBounds(const TRect& bounds) override;
    void setState(ushort aState, bool enable) override;
    void handleEvent(TEvent &ev) override;
    void draw() override;
};

#endif // TVTERM_TERMVIEW_H
