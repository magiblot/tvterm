#ifndef TVTERM_WND_H
#define TVTERM_WND_H

#include <tvterm/termwnd.h>

class TerminalWindow : public tvterm::BasicTerminalWindow
{
    using super = tvterm::BasicTerminalWindow;

public:

    static const tvterm::BasicTerminalWindowAppConstants appConsts;

    TerminalWindow(const TRect &bounds, tvterm::TerminalActivity &aTerm) noexcept;

    void handleEvent(TEvent &ev) override;
};

inline TerminalWindow::TerminalWindow( const TRect &bounds,
                                       tvterm::TerminalActivity &aTerm ) noexcept :
    TWindowInit(&initFrame),
    super(bounds, aTerm, appConsts)
{
}

#endif // TVTERM_WND_H
