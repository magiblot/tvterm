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
struct TerminalState;
struct TVTermConstants;
struct TerminalUpdatedMsg;

class BasicTerminalWindow : public TWindow
{
    const TVTermConstants &consts;
    TerminalView *view {nullptr};
    size_t titleCapacity {0};
    GrowArray termTitle;

    void checkChanges(TerminalUpdatedMsg &) noexcept;
    void resizeTitle(size_t);
    bool updateTitle(TerminalActivity &, TerminalState &) noexcept;

protected:

    bool isClosed() const noexcept;

public:

    static TFrame *initFrame(TRect);

    // Takes ownership over 'term'.
    // The lifetime of 'consts' must exceed that of 'this'.
    // Assumes 'this->TWindow::frame' to be a BasicTerminalFrame.
    BasicTerminalWindow( const TRect &bounds, TerminalActivity &term,
                         const TVTermConstants &consts ) noexcept;

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
