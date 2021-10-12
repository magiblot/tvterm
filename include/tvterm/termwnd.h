#ifndef TVTERM_TERMWND_H
#define TVTERM_TERMWND_H

#include <tvterm/array.h>

#define Uses_TWindow
#define Uses_TCommandSet
#include <tvision/tv.h>

namespace tvterm
{

class TerminalView;
class TerminalActivity;
struct TerminalSharedState;

struct BasicTerminalWindowAppConstants
{
    ushort cmIdle;
    ushort cmGrabInput;
    ushort cmReleaseInput;
    ushort hcInputGrabbed;

    TSpan<const ushort> focusedCmds() const
    {
        return {&cmGrabInput, size_t(&cmReleaseInput + 1 - &cmGrabInput)};
    }
};

class BasicTerminalWindow : public TWindow
{
    const BasicTerminalWindowAppConstants &appConsts;
    TerminalView *view {nullptr};
    size_t titleCapacity {0};
    ByteArray termTitle;
    TPoint lastTermSize {0, 0};

    void checkChanges() noexcept;
    void resizeTitle(size_t);
    bool updateTitle(TerminalActivity &, TerminalSharedState &state) noexcept;

protected:

    bool isClosed() const noexcept;

public:

    static TFrame *initFrame(TRect);

    // Takes ownership over 'aTerm'.
    // The lifetime of 'aAppConsts' must exceed that of 'this'.
    // Assumes 'this->TWindow::frame' to be a BasicTerminalFrame.
    BasicTerminalWindow( const TRect &bounds, TerminalActivity &aTerm,
                         const BasicTerminalWindowAppConstants &aAppConsts ) noexcept;

    void shutDown() override;
    const char *getTitle(short) override;

    void handleEvent(TEvent &ev) override;
    void setState(ushort aState, Boolean enable) override;
    ushort execute() override;

    static TRect viewBounds(const TRect &windowBounds);
    static TPoint viewSize(const TRect &windowBounds);
};

inline TRect BasicTerminalWindow::viewBounds(const TRect &windowBounds)
{
    return TRect(windowBounds).grow(-1, -1);
}

inline TPoint BasicTerminalWindow::viewSize(const TRect &windowBounds)
{
    TRect r = viewBounds(windowBounds);
    return r.b - r.a;
}

} // namespace tvterm

#endif // TVTERM_TERMWND_H
