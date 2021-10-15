#ifndef TVTERM_ASYNCIO_H
#define TVTERM_ASYNCIO_H

#include <chrono>
#include <asio/write.hpp>
#include <asio/buffer.hpp>
#include <asio/dispatch.hpp>
#include <asio/io_context.hpp>
#include <asio/executor_work_guard.hpp>
#include <asio/basic_waitable_timer.hpp>
#include <asio/posix/stream_descriptor.hpp>

template <class T>
class TSpan;

namespace tvterm
{

class AsyncIOClient
{
public:

    virtual void onWaitFinish(int error, bool isTimeout) = 0;

};

class AsyncIO
{
public:

    using clock = std::chrono::steady_clock;
    using time_point = clock::time_point;

private:

    bool waitingForInput {false};
    AsyncIOClient &client;

    asio::io_context io;
    asio::executor_work_guard<decltype(io.get_executor())> work;
    asio::posix::stream_descriptor descriptor;
    asio::basic_waitable_timer<clock> inputWaitTimer;

public:

    // The lifetime of 'aClient' must exceed that of 'this'.
    AsyncIO(AsyncIOClient &aClient, int fd) noexcept;

    void run() noexcept;
    void stop() noexcept;

    template <class Func>
    void dispatch(Func &&func) noexcept;

    void waitInput() noexcept;
    void waitInputUntil(time_point timeout) noexcept;

    bool canReadInput() noexcept;
    size_t readInput(TSpan<char> buf) noexcept;
    template <class Buffer>
    void writeOutput(Buffer &&buf) noexcept;
};

template <class Buffer>
inline void AsyncIO::writeOutput(Buffer &&buf) noexcept
{
    asio::mutable_buffer mb {buf.data(), buf.size()};
    if (mb.size())
        asio::async_write(
            descriptor, mb,
            [buf = std::move(buf)] (auto, auto) noexcept {}
        );
}

template <class Func>
inline void AsyncIO::dispatch(Func &&func) noexcept
{
    asio::dispatch(io, std::move(func));
}

} // namespace tvterm

#endif // TVTERM_ASYNCIO_H
