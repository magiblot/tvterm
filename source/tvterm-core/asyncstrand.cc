#include <tvterm/asyncstrand.h>

#include <tvision/tv.h>

namespace tvterm
{

AsyncStrand::AsyncStrand(AsyncStrandClient &aClient, int fd) noexcept :
    client(aClient),
    work(io.get_executor()),
    descriptor(io, fd),
    inputWaitTimer(io)
{
}

void AsyncStrand::run() noexcept
{
    io.run();
}

void AsyncStrand::stop() noexcept
{
    work.reset();
    io.stop();
}

void AsyncStrand::waitInput() noexcept
{
    if (!waitingForInput)
    {
        waitingForInput = true;
        descriptor.async_wait(
            asio::posix::stream_descriptor::wait_read,
            [this] (auto &error) noexcept {
                waitingForInput = false;
                inputWaitTimer.cancel();
                client.onWaitFinish(error.value(), false);
            }
        );
    }
}

void AsyncStrand::waitInputUntil(time_point timeout) noexcept
{
    waitInput();
    inputWaitTimer.expires_at(timeout);
    inputWaitTimer.async_wait(
        [this] (auto &error) noexcept {
            if (error != asio::error::operation_aborted)
                client.onWaitFinish(error.value(), true);
        }
    );
}

bool AsyncStrand::canReadInput() noexcept
{
    decltype(descriptor)::bytes_readable cmd;
    asio::error_code err;
    descriptor.io_control(cmd, err);
    return !err && cmd.get() > 0;
}

size_t AsyncStrand::readInput(TSpan<char> buf) noexcept
{
    asio::error_code ec;
    return descriptor.read_some(asio::buffer(buf.data(), buf.size()), ec);
}

} // namespace tvterm
