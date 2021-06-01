#define Uses_TEvent
#define Uses_TTimer
#include <tvision/tv.h>

#include <tvterm/ptylisten.h>
#include <tvterm/vtermadapt.h>
#include <tvterm/cmds.h>
#include <sys/ioctl.h>

PTYListener::PTYListener(TVTermAdapter &vterm, asio::io_context &io, int fd) :
    vterm(vterm),
    descriptor(io, fd),
    mAlive(TArc<TMutex<bool>>::make(true)),
    emptyCount(0)
{
    asyncWait();
}


PTYListener::~PTYListener()
{
    mAlive->lock([&] (auto &alive) {
        alive = false;
    });
}

void PTYListener::asyncWait()
{
    descriptor.async_wait(
        asio::posix::stream_descriptor::wait_read,
        [&, mAlive=mAlive] (const asio::error_code& error) mutable {
            mAlive->lock([&] (auto &alive) {
                if (alive)
                    onReady(error);
            });
        }
    );
}

bool PTYListener::streamNotEmpty()
{
    decltype(descriptor)::bytes_readable cmd;
    asio::error_code err;
    descriptor.io_control(cmd, err);
    return !err && cmd.get() > 0;
}

void PTYListener::onReady(const asio::error_code &error)
{
    if (!error && streamNotEmpty())
    {
        emptyCount = 0;
        setTimeout(0, [&, mAlive=mAlive] {
            if (mAlive->get())
            {
                vterm.read();
                asyncWait();
            }
        });
    }
    else if (++emptyCount < maxEmpty)
        // This happens sometimes, especially when running in Valgrind.
        // Give it a few chances before assuming EOF.
        asyncWait();
    else
        setTimeout(0, [&, mAlive=mAlive] {
            if (mAlive->get())
            {
                vterm.pty.close();
            }
        });
}
