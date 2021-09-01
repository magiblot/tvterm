#define Uses_TEvent
#define Uses_TTimer
#include <tvision/tv.h>

#include <tvterm/ptylisten.h>
#include <tvterm/vtermadapt.h>
#include <tvterm/vtermview.h>
#include <tvterm/cmds.h>

thread_local char PTYListener::localBuf alignas(4096) [bufSize];

PTYListener::PTYListener(TVTermAdapter &vterm, asio::io_context &io, int fd) :
    vterm(vterm),
    descriptor(io, fd),
    timer(io),
    mAlive(TArc<TMutex<bool>>::make(true)),
    emptyCount(0)
{
}

void PTYListener::start()
{
    asyncWait();
}

void PTYListener::stop()
{
    mAlive->get() = false;
    descriptor.cancel();
    mAlive->lock([&] (auto &) {});
}

bool PTYListener::streamNotEmpty()
{
    decltype(descriptor)::bytes_readable cmd;
    asio::error_code err;
    descriptor.io_control(cmd, err);
    return !err && cmd.get() > 0;
}

void PTYListener::asyncWait()
{
    descriptor.async_wait(
        asio::posix::stream_descriptor::wait_read,
        [&, mAlive=mAlive] (auto &error) mutable {
            mAlive->lock([&] (auto &alive) {
                if (alive)
                    waitHandler(error);
            });
        }
    );
}

template <class Func>
void PTYListener::asyncReadUntil(time_point timeout, Func &&func)
// 'func' takes a TSpan<char> by parameter.
{
    auto &&done = TArc<bool>::make(false);
    descriptor.async_wait(
        asio::posix::stream_descriptor::wait_read,
        [&, mAlive=mAlive, done, func] (auto &error) mutable {
            mAlive->lock([&] (auto &alive) {
                if (alive && !*done)
                {
                    *done = true;
                    asio::error_code ec;
                    size_t bytes = 0;
                    if (!error && streamNotEmpty())
                        bytes = descriptor.read_some(asio::buffer(localBuf), ec);
                    func(TSpan<const char>(localBuf, bytes));
                }
            });
        }
    );
    timer.expires_at(timeout);
    timer.async_wait(
        [&, mAlive=mAlive, done=std::move(done), func=std::move(func)] (auto &error) mutable {
            mAlive->lock([&] (auto &alive) {
                if (alive && !*done)
                {
                    *done = true;
                    func(TSpan<const char>(nullptr, 0));
                }
            });
        }
    );
}

void PTYListener::waitHandler(const asio::error_code &error)
{
    if (!error && streamNotEmpty())
    {
        emptyCount = 0;
        asio::error_code ec;
        size_t bytes = descriptor.read_some(asio::buffer(localBuf), ec);
        readInput({localBuf, bytes}, clock::now() + maxReadTime);
    }
    else if (++emptyCount < maxEmpty)
        // This happens sometimes, especially when running in Valgrind.
        // Give it a few chances before assuming EOF.
        asyncWait();
    else
    {
        vterm.state |= vtClosed;
        TEventQueue::wakeUp();
    }
}

void PTYListener::readInput(TSpan<const char> buf, time_point limit)
{
    vterm.readInput(buf);
    time_point now;
    if (buf.size() && (now = clock::now()) < limit)
    {
        auto until = ::min(limit, now + waitForFurtherDataDelay);
        asyncReadUntil(
            until,
            [&, limit] (auto buf) {
                readInput(buf, limit);
            }
        );
    }
    else
    {
        vterm.flushDamage();
        vterm.state |= vtUpdated;
        TEventQueue::wakeUp();
        asyncWait();
    }
};
