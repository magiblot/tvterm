#include <tvterm/asyncstrand.h>

#include <tvision/tv.h>

void AsyncStrand::waitInput() noexcept
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
                    if (client)
                        client->onWaitFinish(error.value(), false);
                }
            })
        );
    }
}

void AsyncStrand::waitInputUntil(time_point timeout) noexcept
{
    waitInput();
    inputWaitTimer.expires_at(timeout);
    inputWaitTimer.async_wait(
        asio::bind_executor(strand, [this, alive = alive] (auto &error) noexcept {
            if (*alive && error != asio::error::operation_aborted && client)
                client->onWaitFinish(error.value(), true);
        })
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
