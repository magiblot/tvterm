#include <tvterm/termactiv.h>

#define Uses_TEvent
#include <tvision/tv.h>

static void childActions() noexcept
{
    setenv("TERM", "xterm-256color", 1);
    setenv("COLORTERM", "truecolor", 1);
}

inline TerminalActivity::TerminalActivity( TPoint size, PtyDescriptor ptyDescriptor,
                                           TerminalAdapter &aTerminal,
                                           asio::io_context &io ) noexcept :
    listener(PTYListener::create(size, ptyDescriptor, aTerminal, io))
{
}

TerminalActivity *TerminalActivity::create( TPoint size, TerminalAdapter &terminal,
                                            asio::io_context &io,
                                            void (&onError)(const char *) ) noexcept
{
    auto ptyDescriptor = createPty(size, childActions, onError);
    if (ptyDescriptor.valid())
        return new TerminalActivity(size, ptyDescriptor, terminal, io);
    delete &terminal;
    return nullptr;
}

TerminalActivity::~TerminalActivity()
{
    listener.destroy();
}

TPoint TerminalActivity::getSize() const noexcept
{
    return listener.getSize();
}

void TerminalActivity::changeSize(TPoint size) noexcept
{
    listener.run([&listener = listener, size] {
        listener.changeSize(size);
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
