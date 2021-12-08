#include <tvterm/termactiv.h>
#include <tvterm/threadpool.h>

#define Uses_TEvent
#include <tvision/tv.h>

namespace tvterm
{

inline TerminalActivity::TerminalActivity( TPoint size,
                                           TerminalAdapter &(&createTerminal)(TPoint, TerminalSharedState &),
                                           PtyDescriptor ptyDescriptor,
                                           ThreadPool &threadPool ) noexcept :
    pty(ptyDescriptor),
    async(*this, pty.getMaster()),
    terminal(createTerminal(size, sharedState))
{
    threadPool.run([&, g = std::unique_ptr<TerminalActivity>(this)] () noexcept {
        async.run();
    });

    async.waitInput();
}

TerminalActivity *TerminalActivity::create( TPoint size,
                                            TerminalAdapter &(&createTerminal)(TPoint, TerminalSharedState &),
                                            void (&childActions)(),
                                            void (&onError)(const char *),
                                            ThreadPool &threadPool,
                                            const PtyUtil &ptyUtil ) noexcept
{
    auto ptyDescriptor = createPty(size, childActions, onError, ptyUtil);
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

void TerminalActivity::onWaitFinish(int error, bool isTimeout) noexcept
{
    advanceWaitState(error, isTimeout);
}

void TerminalActivity::advanceWaitState(int error, bool isTimeout) noexcept
{
    using std::chrono::milliseconds;
    switch (waitState)
    {
        case wsReady:
            if (!error && async.canReadInput())
            {
                consecutiveEOF = 0;
                readTimeout = clock::now() + milliseconds(maxReadTimeMs);
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
                    getState([&] (auto &state) {
                        terminal.receive({buf, bytes}, state);
                    });
                }
                else
                    return async.waitInputUntil(::min(readTimeout, now + milliseconds(inputWaitStepMs)));
            }
            else
                waitState = wsFlush;
            break;
        }
        case wsFlush:
            getState([&] (auto &state) {
                terminal.flushDamage(state);
            });
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
        getState([&] (auto &state) {
            terminal.setSize(viewSize, state);
        });
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

void TerminalActivity::sendFocus(bool focus) noexcept
{
    async.dispatch([this, focus] {
        terminal.setFocus(focus);
        async.writeOutput(terminal.takeWriteBuffer());
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
