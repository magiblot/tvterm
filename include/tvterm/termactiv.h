#ifndef TVTERM_TERMACTIV_H
#define TVTERM_TERMACTIV_H

#include <tvterm/io.h>
#include <tvterm/pty.h>
#include <tvterm/termadapt.h>
#include <tvterm/ptylisten.h>

#include <utility>

struct KeyDownEvent;
struct MouseEventType;

class TerminalActivity
{

    PTYListener &listener;

    TerminalActivity( TPoint size, PtyDescriptor ptyDescriptor,
                      TerminalAdapter &aTerminal, asio::io_context &io ) noexcept;

public:

    static TerminalActivity *create( TPoint size, TerminalAdapter &terminal,
                                     asio::io_context &io,
                                     void (&onError)(const char *reason) ) noexcept;
    ~TerminalActivity();

    TPoint getSize() const noexcept;
    void changeSize(TPoint aSize) noexcept;

    void handleKeyDown(const KeyDownEvent &keyDown) noexcept;
    void handleMouse(ushort what, const MouseEventType &mouse) noexcept;

    bool checkChanges() noexcept;
    bool isClosed() const noexcept;

    template <class Func>
    // This method locks a mutex, so don't invoke it again from within the callback.
    // * 'func' takes a 'TerminalReceivedState &' by parameter.
    auto getState(Func &&func) noexcept;

};

inline bool TerminalActivity::checkChanges() noexcept
{
    return listener.checkChanges();
}

inline bool TerminalActivity::isClosed() const noexcept
{
    return listener.isClosed();
}

template <class Func>
inline auto TerminalActivity::getState(Func &&func) noexcept
{
    return listener.terminal.getState(std::move(func));
}

#endif // TVTERM_TERMCONN_H
