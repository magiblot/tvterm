#ifndef TVTERM_TERMWND_H
#define TVTERM_TERMWND_H

#define Uses_TWindow
#define Uses_TCommandSet
#include <tvision/tv.h>
#include <vector>

class TerminalView;
class TerminalActivity;
struct TerminalReceivedState;

class TerminalWindow : public TWindow
{
    TerminalView *view {nullptr};
    size_t titleCapacity {0};
    std::vector<char> termTitle;

    static TFrame *initFrame(TRect);

    void checkChanges() noexcept;
    void resizeTitle(size_t);
    bool updateTitle(TerminalActivity &, TerminalReceivedState &state) noexcept;
    bool isClosed() const noexcept;

public:

    static TCommandSet focusedCmds;

    TerminalWindow(const TRect &bounds, TerminalActivity &aTerm);

    void shutDown() override;
    const char *getTitle(short) override;

    void handleEvent(TEvent &ev) override;
    void setState(ushort aState, Boolean enable) override;
    ushort execute() override;

    static TRect viewBounds(const TRect &windowBounds);
    static TPoint viewSize(const TRect &windowBounds);
};

inline TRect TerminalWindow::viewBounds(const TRect &windowBounds)
{
    return TRect(windowBounds).grow(-1, -1);
}

inline TPoint TerminalWindow::viewSize(const TRect &windowBounds)
{
    TRect r = viewBounds(windowBounds);
    return r.b - r.a;
}

#endif // TVTERM_TERMWND_H
