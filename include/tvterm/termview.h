#ifndef TVTERM_TERMVIEW_H
#define TVTERM_TERMVIEW_H

#define Uses_TView
#define Uses_TGroup
#include <tvision/tv.h>

struct MouseEventType;

namespace tvterm
{

class TerminalController;
class TerminalSurface;
struct TerminalState;
struct TVTermConstants;

class TerminalView : public TView
{
    const TVTermConstants &consts;
    bool ownerBufferChanged {false};

    // Selection state (main thread only, no sync needed)
    struct Selection
    {
        TPoint anchor {0, 0};
        TPoint current {0, 0};
        bool active {false};    // drag in progress
        bool valid {false};     // completed selection exists
    };

    Selection selection;
    bool cachedMouseEnabled {false};

    void handleMouse(ushort what, MouseEventType mouse) noexcept;
    void handleSelectionMouse(ushort what, TPoint localPos) noexcept;
    void updateCursor(TerminalState &state) noexcept;
    void updateDisplay(TerminalSurface &surface) noexcept;
    void applySelectionHighlight(TerminalSurface &surface) noexcept;
    void normalizeSelection(TPoint &start, TPoint &end) const noexcept;
    void clearSelection() noexcept;
    bool canReuseOwnerBuffer() noexcept;

public:

    TerminalController &termCtrl;

    // Takes ownership over 'termCtrl'.
    // The lifetime of 'consts' must exceed that of 'this'.
    TerminalView( const TRect &bounds, TerminalController &termCtrl,
                  const TVTermConstants &consts ) noexcept;
    ~TerminalView();

    void changeBounds(const TRect& bounds) override;
    void setState(ushort aState, bool enable) override;
    void handleEvent(TEvent &ev) override;
    void draw() override;

    void copySelection() noexcept;
};

} // namespace tvterm

#endif // TVTERM_TERMVIEW_H
