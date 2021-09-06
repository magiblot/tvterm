#include <tvterm/termactiv.h>

#define Uses_TEvent
#include <tvision/tv.h>

static void childActions() noexcept
{
    setenv("TERM", "xterm-256color", 1);
    setenv("COLORTERM", "truecolor", 1);
}

inline TerminalActivity::TerminalActivity( TerminalAdapter &aTerminal,
                                           asio::io_context &io,
                                           PtyDescriptor ptyDescriptor ) noexcept :
    pty(ptyDescriptor),
    listener(PTYListener::create(aTerminal, io, pty.getMaster()))
{
}

TerminalActivity *TerminalActivity::create( TPoint size, TerminalAdapter &terminal,
                                            asio::io_context &io,
                                            void (&onError)(const char *) ) noexcept
{
    auto ptyDescriptor = createPty(size, childActions, onError);
    if (ptyDescriptor.valid())
        return new TerminalActivity(terminal, io, ptyDescriptor);
    delete &terminal;
    return nullptr;
}

TerminalActivity::~TerminalActivity()
{
    listener.destroy();
}

TPoint TerminalActivity::getSize() const noexcept
{
    return pty.getSize();
}

void TerminalActivity::changeSize(TPoint size) noexcept
{
    pty.setSize(size);
    listener.run([&listener = listener, size] {
        listener.terminal.changeSize(size);
    });
}

void TerminalActivity::handleKeyDown(const KeyDownEvent &keyDown) noexcept
{
    listener.run([&listener = listener, keyDown] {
        auto &terminal = listener.terminal;
        terminal.handleKeyDown(keyDown);
        listener.writeOutput(terminal.takeWriteBuffer());
    });
}

void TerminalActivity::handleMouse(ushort what, const MouseEventType &mouse) noexcept
{
    listener.run([&listener = listener, what, mouse] {
        auto &terminal = listener.terminal;
        terminal.handleMouse(what, mouse);
        listener.writeOutput(terminal.takeWriteBuffer());
    });
}
