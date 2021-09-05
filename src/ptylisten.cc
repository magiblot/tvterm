#include <tvterm/ptylisten.h>
#include <tvterm/termadapt.h>

#define Uses_TEventQueue
#include <tvision/tv.h>

thread_local char PTYListener::staticBuf alignas(4096) [bufSize];

inline PTYListener::PTYListener( TerminalAdapter &aTerminal,
                                 asio::io_context &io, int fd ) noexcept :
    strand(io),
    descriptor(io, fd),
    timer(io),
    terminal(aTerminal)
{
}

PTYListener &PTYListener::create( TerminalAdapter &aTerminal,
                                  asio::io_context &io, int fd ) noexcept
{
    auto &self = *new PTYListener(aTerminal, io, fd);
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
        [s = std::unique_ptr<PTYListener>(this)] {}
    );
}

bool PTYListener::streamNotEmpty()
{
    decltype(descriptor)::bytes_readable cmd;
    asio::error_code err;
    descriptor.io_control(cmd, err);
    return !err && cmd.get() > 0;
}

void PTYListener::waitInput()
{
    descriptor.async_wait(
        asio::posix::stream_descriptor::wait_read,
        asio::bind_executor(strand, [this, alive = alive] (auto &error) {
            if (*alive)
                handleReadableInput(error);
        })
    );
}

template <class Func>
inline void PTYListener::readInputUntil(time_point timeout, Func &&func)
// 'func' takes a TSpan<char> by parameter.
{
    auto &done = *new bool(false);
    descriptor.async_wait(
        asio::posix::stream_descriptor::wait_read,
        asio::bind_executor(strand, [this, alive = alive, &done, func] (auto &error) {
            if (!done)
            {
                done = true;
                if (*alive)
                {
                    asio::error_code ec;
                    size_t bytes = 0;
                    if (!error && streamNotEmpty())
                        bytes = descriptor.read_some(asio::buffer(staticBuf), ec);
                    func(TSpan<const char>(staticBuf, bytes));
                }
            }
            else
                delete &done;
        })
    );
    timer.expires_at(timeout);
    timer.async_wait(
        asio::bind_executor(strand, [this, alive = alive, &done, func = std::move(func)] (auto &error) {
            if (!done)
            {
                done = true;
                if (*alive)
                    func(TSpan<const char>(nullptr, 0));
            }
            else
                delete &done;
        })
    );
}

void PTYListener::handleReadableInput(const asio::error_code &error)
{
    TimeLogger::log("PTYListener@%p: handleReadableInput(%d).", this, error.value());
    if (!error && streamNotEmpty())
    {
        consecutiveEOF = 0;
        asio::error_code ec;
        size_t bytes = descriptor.read_some(asio::buffer(staticBuf), ec);
        doReadCycle({staticBuf, bytes}, clock::now() + maxReadTime);
    }
    else if (++consecutiveEOF < maxConsecutiveEOF)
    {
        // This happens sometimes, especially when running in Valgrind.
        // Give it a few chances before assuming EOF.
        waitInput();
    }
    else
    {
        reachedEOF = true;
        TEventQueue::wakeUp();
    }
}

void PTYListener::doReadCycle(TSpan<const char> buf, time_point limit)
{
    terminal.receive(buf);
    time_point now;
    if (buf.size() && (now = clock::now()) < limit)
    {
        auto until = ::min(limit, now + readWaitStep);
        readInputUntil(
            until,
            [this, limit] (auto buf) {
                doReadCycle(buf, limit);
            }
        );
    }
    else
    {
        terminal.flushDamage();
        updated = true;
        TEventQueue::wakeUp();
        writeOutput(terminal.takeWriteBuffer());
        waitInput();
    }
};

void PTYListener::writeOutput(std::vector<char> &&buf)
{
    if (buf.size())
        asio::async_write(
            descriptor,
            asio::buffer(buf.data(), buf.size()),
            asio::bind_executor(strand, [buf = std::move(buf)] (...) {})
        );
}
