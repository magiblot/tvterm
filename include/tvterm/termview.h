#ifndef TVTERM_TERMVIEW_H
#define TVTERM_TERMVIEW_H

#define Uses_TView
#define Uses_TGroup
#include <tvision/tv.h>

struct MouseEventType;

namespace tvterm
{

class TerminalActivity;
class TerminalSurface;
struct TerminalState;
struct TVTermConstants;

class TerminalView : public TView
{
    const TVTermConstants &consts;
    bool ownerBufferChanged {false};

    void handleMouse(ushort what, MouseEventType mouse) noexcept;
    void updateCursor(TerminalState &state) noexcept;
    void updateDisplay(TerminalSurface &surface) noexcept;
    bool canReuseOwnerBuffer() noexcept;

public:

    TerminalActivity &term;

    // Takes ownership over 'term'.
    // The lifetime of 'consts' must exceed that of 'this'.
    TerminalView( const TRect &bounds, TerminalActivity &term,
                  const TVTermConstants &consts ) noexcept;
    ~TerminalView();

    void changeBounds(const TRect& bounds) override;
    void setState(ushort aState, bool enable) override;
    void handleEvent(TEvent &ev) override;
    void draw() override;
};

} // namespace tvterm

#endif // TVTERM_TERMVIEW_H
