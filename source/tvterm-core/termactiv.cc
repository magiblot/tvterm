#include <tvterm/termactiv.h>
#include <tvterm/threadpool.h>

#define Uses_TEvent
#include <tvision/tv.h>

namespace tvterm
{

inline TerminalActivity::TerminalActivity( TPoint size,
                                           TerminalEmulatorFactory &terminalEmulatorFactory,
                                           PtyDescriptor ptyDescriptor,
                                           ThreadPool &threadPool ) noexcept :
    pty(ptyDescriptor),
    async(*this, pty.getMaster()),
    terminal(terminalEmulatorFactory.create(size, clientDataWriter))
{
    threadPool.run([&, ownership = std::unique_ptr<TerminalActivity>(this)] () noexcept {
        async.run();
    });
}

TerminalActivity *TerminalActivity::create( TPoint size,
                                            TerminalEmulatorFactory &terminalEmulatorFactory,
                                            void (&onError)(const char *),
                                            ThreadPool &threadPool ) noexcept
{
    auto ptyDescriptor = createPty(size, terminalEmulatorFactory.getCustomEnvironment(), onError);
    if (ptyDescriptor.valid())
        return new TerminalActivity(size, terminalEmulatorFactory, ptyDescriptor, threadPool);
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

                            TerminalEvent event;
                            event.type = TerminalEventType::ClientDataRead;
                            event.clientDataRead = {buf, bytes};
                            terminal.handleEvent(event);
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
                terminalState.lock([&] (auto &state) {
                    terminal.updateState(state);
                });
                checkSize();
                updated = true;
                TEvent::putNothing();
                if (!isError)
                {
                    async.writeOutput(std::move(clientDataWriter.buffer));
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

        TerminalEvent event;
        event.type = TerminalEventType::ViewportResize;
        event.viewportResize = {viewportSize.x, viewportSize.y};
        terminal.handleEvent(event);

        if (isClosed())
        {
            terminalState.lock([&] (auto &state) {
                terminal.updateState(state);
            });
            updated = true;
            TEvent::putNothing();
        }
        else
            pty.setSize(viewportSize);
    }
}

void TerminalActivity::sendEvent(const TerminalEvent &event) noexcept
{
    async.dispatch([this, event] {
        switch (event.type)
        {
            case TerminalEventType::ViewportResize:
                viewportSizeChanged = true;
                viewportSize = {event.viewportResize.x, event.viewportResize.y};
                checkSize();
                break;

            default:
                terminal.handleEvent(event);
                async.writeOutput(std::move(clientDataWriter.buffer));
                break;
        }
    });
}

} // namespace tvterm
