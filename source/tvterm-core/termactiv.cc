#include <tvterm/termactiv.h>
#include <tvterm/threadpool.h>

#define Uses_TEvent
#include <tvision/tv.h>

namespace tvterm
{

inline TerminalActivity::TerminalActivity( TPoint size,
                                           TerminalAdapterFactory &createTerminal,
                                           PtyDescriptor ptyDescriptor,
                                           ThreadPool &threadPool ) noexcept :
    pty(ptyDescriptor),
    async(*this, pty.getMaster()),
    terminal(createTerminal(size, outputBuffer, sharedState))
{
    threadPool.run([&, ownership = std::unique_ptr<TerminalActivity>(this)] () noexcept {
        async.run();
    });
}

TerminalActivity *TerminalActivity::create( TPoint size,
                                            TerminalAdapterFactory &createTerminal,
                                            void (&childActions)(),
                                            void (&onError)(const char *),
                                            ThreadPool &threadPool ) noexcept
{
    auto ptyDescriptor = createPty(size, childActions, onError);
    if (ptyDescriptor.valid())
        return new TerminalActivity(size, createTerminal, ptyDescriptor, threadPool);
    return nullptr;
}

inline TerminalActivity::~TerminalActivity()
{
    delete &terminal;
}

void TerminalActivity::destroy() noexcept
{
    async.stop();
}

bool TerminalActivity::onWaitFinish(bool isError, bool isTimeout) noexcept
{
    using std::chrono::milliseconds;
    if (isError && waitState != wsEOF)
        waitState = wsFlush;
    while (true)
    {
        switch (waitState)
        {
            case wsInitial:
                if (consecutiveEOF < maxConsecutiveEOF)
                {
                    readTimeout = Clock::now() + milliseconds(maxReadTimeMs);
                    waitState = wsRead;
                }
                else
                    waitState = wsEOF;
                break;
            case wsRead:
            {
                if (!isTimeout)
                {
                    TimePoint now;
                    while ((now = Clock::now()) < readTimeout)
                    {
                        static thread_local char buf alignas(4096) [bufSize];
                        if (size_t bytes = async.readInput({buf, bufSize}))
                        {
                            consecutiveEOF = -1;
                            terminal.receive({buf, bytes});
                        }
                        else if (++consecutiveEOF < maxConsecutiveEOF)
                        {
                            async.setWaitTimeout(
                                ::min<Duration>(milliseconds(inputWaitStepMs), readTimeout - now)
                            );
                            return true;
                        }
                        else
                            break;
                    }
                }
                waitState = wsFlush;
                break;
            }
            case wsFlush:
                terminal.flushDamage();
                checkSize();
                updated = true;
                TEvent::putNothing();
                if (!isError)
                {
                    async.writeOutput(std::move(outputBuffer));
                    waitState = wsInitial;
                    return true;
                }
                waitState = wsEOF;
                break;
            case wsEOF:
                updated = true;
                closed = true;
                TEvent::putNothing();
                return false;
        }
    }
}

void TerminalActivity::checkSize() noexcept
{
    if (waitState != wsRead && viewportSizeChanged)
    {
        viewportSizeChanged = false;
        terminal.setSize(viewportSize);
        if (isClosed())
        {
            terminal.flushDamage();
            updated = true;
            TEvent::putNothing();
        }
        else
            pty.setSize(viewportSize);
    }
}

void TerminalActivity::sendResize(TPoint aSize) noexcept
{
    async.dispatch([this, aSize] {
        viewportSizeChanged = true;
        viewportSize = aSize;
        checkSize();
    });
}

void TerminalActivity::sendFocus(bool focus) noexcept
{
    async.dispatch([this, focus] {
        terminal.setFocus(focus);
        async.writeOutput(std::move(outputBuffer));
    });
}

void TerminalActivity::sendKeyDown(const KeyDownEvent &keyDown) noexcept
{
    async.dispatch([this, keyDown] {
        terminal.handleKeyDown(keyDown);
        async.writeOutput(std::move(outputBuffer));
    });
}

void TerminalActivity::sendMouse(ushort what, const MouseEventType &mouse) noexcept
{
    async.dispatch([this, what, mouse] {
        terminal.handleMouse(what, mouse);
        async.writeOutput(std::move(outputBuffer));
    });
}

} // namespace tvterm
