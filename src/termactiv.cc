#include <tvterm/termactiv.h>

#define Uses_TEventQueue
#include <tvision/tv.h>

inline AsyncIOStrand::AsyncIOStrand(asio::io_context &io, int fd) noexcept :
    strand(io),
    descriptor(io, fd),
    inputWaitTimer(io)
{
}

inline AsyncIOStrand::~AsyncIOStrand()
{
    *alive = false;
}

bool AsyncIOStrand::canReadInput() noexcept
{
    decltype(descriptor)::bytes_readable cmd;
    asio::error_code err;
    descriptor.io_control(cmd, err);
    return !err && cmd.get() > 0;
}

void AsyncIOStrand::waitInput() noexcept
{
    if (!waitingForInput)
    {
        waitingForInput = true;
        descriptor.async_wait(
            asio::posix::stream_descriptor::wait_read,
            asio::bind_executor(strand, [this, alive = alive] (auto &error) noexcept {
                if (*alive)
                {
                    waitingForInput = false;
                    inputWaitTimer.cancel();
                    onWaitFinish(error.value(), false);
                }
            })
        );
    }
}

void AsyncIOStrand::waitInputUntil(time_point timeout) noexcept
{
    waitInput();
    inputWaitTimer.expires_at(timeout);
    inputWaitTimer.async_wait(
        asio::bind_executor(strand, [this, alive = alive] (auto &error) noexcept {
            if (*alive && error != asio::error::operation_aborted)
                onWaitFinish(error.value(), true);
        })
    );
}

template <class Buffer>
inline void AsyncIOStrand::writeOutput(Buffer &&buf) noexcept
{
    if (buf.size())
        asio::async_write(
            descriptor,
            asio::buffer(buf.data(), buf.size()),
            asio::bind_executor(strand, [buf = std::move(buf)] (...) noexcept {})
        );
}

size_t AsyncIOStrand::readInput(TSpan<char> buf) noexcept
{
    asio::error_code ec;
    return descriptor.read_some(asio::buffer(buf.data(), buf.size()), ec);
}

template <class Func>
inline void AsyncIOStrand::dispatch(Func &&func) noexcept
{
    asio::dispatch(strand, std::move(func));
}

inline TerminalActivity::TerminalActivity( PtyDescriptor ptyDescriptor,
                                 TerminalAdapter &aTerminal, asio::io_context &io ) noexcept :
    AsyncIOStrand(io, ptyDescriptor.master_fd),
    pty(ptyDescriptor),
    terminal(aTerminal)
{
    waitInput();
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
    dispatch([s = std::unique_ptr<TerminalActivity>(this)] () noexcept {});
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
            if (!error && canReadInput())
            {
                consecutiveEOF = 0;
                readTimeout = clock::now() + maxReadTime;
                waitState = wsRead;
            }
            else if (++consecutiveEOF < maxConsecutiveEOF)
                // This happens sometimes, especially when running in Valgrind.
                // Give it a few chances before assuming EOF.
                return waitInput();
            else
                waitState = wsEOF;
            break;
        case wsRead:
        {
            time_point now;
            if (!isTimeout && (now = clock::now()) < readTimeout)
            {
                if (canReadInput())
                {
                    static thread_local char buf alignas(4096) [bufSize];
                    size_t bytes = readInput(buf);
                    terminal.receive({buf, bytes});
                }
                else
                    return waitInputUntil(::min(readTimeout, now + inputWaitStep));
            }
            else
                waitState = wsFlush;
            break;
        }
        case wsFlush:
            terminal.flushDamage();
            checkSize();
            updated = true;
            TEventQueue::wakeUp();
            writeOutput(terminal.takeWriteBuffer());
            waitState = wsReady;
            return waitInput();
        case wsEOF:
            TEventQueue::wakeUp();
            return;
    }
    advanceWaitState(0, false);
}

void TerminalActivity::checkSize() noexcept
{
    if (waitState != wsRead && viewSizeChanged)
    {
        viewSizeChanged = false;
        pty.setSize(viewSize);
        terminal.setSize(viewSize);
    }
}

void TerminalActivity::changeSize(TPoint aSize) noexcept
{
    dispatch([this, aSize] {
        viewSizeChanged = true;
        viewSize = aSize;
        checkSize();
    });
}

void TerminalActivity::sendKeyDown(const KeyDownEvent &keyDown) noexcept
{
    dispatch([this, keyDown] {
        terminal.handleKeyDown(keyDown);
        writeOutput(terminal.takeWriteBuffer());
    });
}

void TerminalActivity::sendMouse(ushort what, const MouseEventType &mouse) noexcept
{
    dispatch([this, what, mouse] {
        terminal.handleMouse(what, mouse);
        writeOutput(terminal.takeWriteBuffer());
    });
}
