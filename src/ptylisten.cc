#include <tvterm/ptylisten.h>
#include <tvterm/termadapt.h>

#define Uses_TEventQueue
#include <tvision/tv.h>

inline PTYListener::PTYListener( TPoint size, PtyDescriptor ptyDescriptor,
                                 TerminalAdapter &aTerminal, asio::io_context &io ) noexcept :

    pty(ptyDescriptor),
    strand(io),
    descriptor(io, pty.getMaster()),
    inputWaitTimer(io),
    viewSize(size),
    terminal(aTerminal)
{
}

PTYListener &PTYListener::create( TPoint size, PtyDescriptor ptyDescriptor,
                                  TerminalAdapter &aTerminal, asio::io_context &io ) noexcept
{
    auto &self = *new PTYListener(size, ptyDescriptor, aTerminal, io);
    self.waitInput();
    return self;
}

inline PTYListener::~PTYListener()
{
    *alive = false;
    delete &terminal;
}

void PTYListener::destroy()
{
    asio::dispatch(
        strand,
        [s = std::unique_ptr<PTYListener>(this)] () noexcept {}
    );
}

bool PTYListener::streamIsNotEmpty() noexcept
{
    decltype(descriptor)::bytes_readable cmd;
    asio::error_code err;
    descriptor.io_control(cmd, err);
    return !err && cmd.get() > 0;
}

void PTYListener::waitInput() noexcept
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
                    advanceWaitState(error.value(), false);
                }
            })
        );
    }
}

void PTYListener::waitInputUntil(time_point timeout) noexcept
{
    waitInput();
    inputWaitTimer.expires_at(timeout);
    inputWaitTimer.async_wait(
        asio::bind_executor(strand, [this, alive = alive, timeout] (auto &error) noexcept {
            if (*alive && error != asio::error::operation_aborted)
                advanceWaitState(0, true);
        })
    );
}

void PTYListener::advanceWaitState(int error, bool isTimeout) noexcept
{
    switch (waitState)
    {
        case wsReady:
            if (!error && streamIsNotEmpty())
            {
                consecutiveEOF = 0;
                readTimeout = clock::now() + maxReadTime;
                waitState = wsRead;
            }
            else if (++consecutiveEOF < maxConsecutiveEOF)
            {
                // This happens sometimes, especially when running in Valgrind.
                // Give it a few chances before assuming EOF.
                waitInput();
                return;
            }
            else
            {
                waitState = wsEOF;
                return;
            }
            break;
        case wsRead:
        {
            time_point now;
            if (!isTimeout && (now = clock::now()) < readTimeout)
            {
                if (streamIsNotEmpty())
                {
                    static thread_local char buf alignas(4096) [bufSize];
                    asio::error_code ec;
                    size_t bytes = descriptor.read_some(asio::buffer(buf), ec);
                    terminal.receive({buf, bytes});
                }
                else
                {
                    waitInputUntil(::min(readTimeout, now + inputWaitStep));
                    return;
                }
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
            waitInput();
            return;
        case wsEOF:
            TEventQueue::wakeUp();
            return;
    }
    advanceWaitState(0, false);
}

void PTYListener::writeOutput(std::vector<char> &&buf) noexcept
{
    if (buf.size())
        asio::async_write(
            descriptor,
            asio::buffer(buf.data(), buf.size()),
            asio::bind_executor(strand, [buf = std::move(buf)] (...) noexcept {})
        );
}

void PTYListener::checkSize() noexcept
{
    if (waitState != wsRead)
    {
        pty.setSize(viewSize);
        terminal.setSize(viewSize);
    }
}
