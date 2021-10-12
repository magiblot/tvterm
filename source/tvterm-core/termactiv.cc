#include <tvterm/termactiv.h>

#define Uses_TEvent
#include <tvision/tv.h>

namespace tvterm
{

inline TerminalActivity::TerminalActivity( PtyDescriptor ptyDescriptor,
                                           TerminalAdapter &aTerminal,
                                           asio::io_context &io ) noexcept :
    pty(ptyDescriptor),
    terminal(aTerminal),
    async(io, pty.getMaster())
{
    async.setClient(this);
    async.waitInput();
}

TerminalActivity *TerminalActivity::create( TPoint size, TerminalAdapter &terminal,
                                            asio::io_context &io,
                                            void (&onError)(const char *) ) noexcept
{
    auto ptyDescriptor = createPty(size, terminal.getChildActions(), onError);
    if (ptyDescriptor.valid())
        return new TerminalActivity(ptyDescriptor, terminal, io);
    delete &terminal;
    return nullptr;
}

inline TerminalActivity::~TerminalActivity()
{
    delete &terminal;
}

void TerminalActivity::destroy() noexcept
{
    async.dispatch([s = std::unique_ptr<TerminalActivity>(this)] () noexcept {});
}

void TerminalActivity::onWaitFinish(int error, bool isTimeout) noexcept
{
    advanceWaitState(error, isTimeout);
}

void TerminalActivity::advanceWaitState(int error, bool isTimeout) noexcept
{
    switch (waitState)
    {
        case wsReady:
            if (!error && async.canReadInput())
            {
                consecutiveEOF = 0;
                readTimeout = clock::now() + maxReadTime;
                waitState = wsRead;
            }
            else if (++consecutiveEOF < maxConsecutiveEOF)
                // This happens sometimes, especially when running in Valgrind.
                // Give it a few chances before assuming EOF.
                return async.waitInput();
            else
                waitState = wsEOF;
            break;
        case wsRead:
        {
            time_point now;
            if (!isTimeout && (now = clock::now()) < readTimeout)
            {
                if (async.canReadInput())
                {
                    static thread_local char buf alignas(4096) [bufSize];
                    size_t bytes = async.readInput(buf);
                    terminal.receive({buf, bytes});
                }
                else
                    return async.waitInputUntil(::min(readTimeout, now + inputWaitStep));
            }
            else
                waitState = wsFlush;
            break;
        }
        case wsFlush:
            terminal.flushDamage();
            checkSize();
            updated = true;
            TEvent::putNothing();
            async.writeOutput(terminal.takeWriteBuffer());
            waitState = wsReady;
            return async.waitInput();
        case wsEOF:
            updated = true;
            TEvent::putNothing();
            return;
    }
    advanceWaitState(0, false);
}

void TerminalActivity::checkSize() noexcept
{
    if (waitState != wsRead && viewSizeChanged)
    {
        viewSizeChanged = false;
        terminal.setSize(viewSize);
        if (isClosed())
            updated = true;
        else
            pty.setSize(viewSize);
    }
}

void TerminalActivity::changeSize(TPoint aSize) noexcept
{
    async.dispatch([this, aSize] {
        viewSizeChanged = true;
        viewSize = aSize;
        checkSize();
    });
}

void TerminalActivity::sendKeyDown(const KeyDownEvent &keyDown) noexcept
{
    async.dispatch([this, keyDown] {
        terminal.handleKeyDown(keyDown);
        async.writeOutput(terminal.takeWriteBuffer());
    });
}

void TerminalActivity::sendMouse(ushort what, const MouseEventType &mouse) noexcept
{
    async.dispatch([this, what, mouse] {
        terminal.handleMouse(what, mouse);
        async.writeOutput(terminal.takeWriteBuffer());
    });
}

} // namespace tvterm
