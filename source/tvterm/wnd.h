#ifndef TVTERM_WND_H
#define TVTERM_WND_H

#include <tvterm/termwnd.h>
#include <tvterm/consts.h>

class TerminalWindow : public tvterm::BasicTerminalWindow
{
public:

    static const tvterm::TVTermConstants appConsts;

    TerminalWindow(const TRect &bounds, tvterm::TerminalController &aTerm) noexcept;

    void handleEvent(TEvent &ev) override;
    void sizeLimits(TPoint &min, TPoint &max) override;

private:

    using Super = tvterm::BasicTerminalWindow;

    void zoom() noexcept;
};

inline TerminalWindow::TerminalWindow( const TRect &bounds,
                                       tvterm::TerminalController &aTerm ) noexcept :
    TWindowInit(&initFrame),
    Super(bounds, aTerm, appConsts)
{
}

#endif // TVTERM_WND_H
