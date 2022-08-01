#include <tvterm/asyncio.h>
#include <tvterm/array.h>

#include <tvision/tv.h>

#include <asio/write.hpp>
#include <asio/buffer.hpp>

namespace tvterm
{

AsyncIO::AsyncIO(AsyncIOClient &aClient, int fd) noexcept :
    client(aClient),
    work(io.get_executor()),
    descriptor(io, fd),
    inputWaitTimer(io)
{
}

void AsyncIO::run() noexcept
{
    waitInput();
    io.run();
}

void AsyncIO::stop() noexcept
{
    work.reset();
    io.stop();
}

void AsyncIO::waitInput() noexcept
{
    descriptor.async_wait(
        asio::posix::stream_descriptor::wait_read,
        [this] (auto &error) noexcept {
            inputWaitTimer.cancel();
            if (client.onWaitFinish(!!error, false))
                waitInput();
        }
    );
}

void AsyncIO::setWaitTimeout(Duration duration) noexcept
{
    inputWaitTimer.expires_after(duration);
    inputWaitTimer.async_wait(
        [this] (auto &error) noexcept {
            if (error != asio::error::operation_aborted)
                client.onWaitFinish(!!error, true);
        }
    );
}

size_t AsyncIO::readInput(TSpan<char> buf) noexcept
{
    decltype(descriptor)::bytes_readable cmd;
    asio::error_code error;
    descriptor.io_control(cmd, error);
    if (!error && cmd.get() > 0)
        return descriptor.read_some(asio::buffer(buf.data(), buf.size()), error);
    return 0;
}

void AsyncIO::writeOutput(GrowArray &&buf) noexcept
{
    asio::mutable_buffer mb {buf.data(), buf.size()};
    if (mb.size())
        asio::async_write(
            descriptor, mb,
            [buf = std::move(buf)] (auto, auto) noexcept {}
        );
}

} // namespace tvterm
