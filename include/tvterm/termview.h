#ifndef TVTERM_TERMVIEW_H
#define TVTERM_TERMVIEW_H

#define Uses_TView
#define Uses_TGroup
#include <tvision/tv.h>

struct MouseEventType;
class TScrollBar;

namespace tvterm
{

class TerminalController;
class TerminalSurface;
struct TerminalState;
struct TVTermConstants;

class TerminalView : public TView
{
    const TVTermConstants &consts;
    TScrollBar *scrollBar {nullptr};
    bool ownerBufferChanged {false};

    SelectionRange selection {};
    SelectionRange prevSelection {};
    bool inSelectionMode {false};

    void handleMouse(ushort what, MouseEventType mouse) noexcept;
    void updateHelpContext() noexcept;
    void updateCursor(TerminalState &state) noexcept;
    void updateScrollBar(TerminalState &state) noexcept;
    void updateDisplay(TerminalState &state) noexcept;
    bool canReuseOwnerBuffer() noexcept;
    TPoint getCursorDisplayedPos(TerminalState &state) noexcept;

    void startSelection(SelectionAnchor pos = {}) noexcept;
    void extendSelection(SelectionAnchor pos) noexcept;
    void cancelSelection() noexcept;
    void copySelection() noexcept;

public:

    TerminalController &termCtrl;

    // Takes ownership over 'termCtrl'.
    // The lifetime of 'consts' must exceed that of 'this'.
    // 'scrollBar' is a non-owning reference.
    TerminalView( const TRect &bounds, TerminalController &termCtrl,
                  const TVTermConstants &consts, TScrollBar *scrollBar ) noexcept;
    ~TerminalView();

    void shutDown() override;
    void changeBounds(const TRect& bounds) override;
    void setState(ushort aState, bool enable) override;
    void handleEvent(TEvent &ev) override;
    void draw() override;
};

} // namespace tvterm

#endif // TVTERM_TERMVIEW_H
