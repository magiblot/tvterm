#ifndef TVTERM_ASYNCIO_H
#define TVTERM_ASYNCIO_H

#include <asio/version.hpp>

#if ASIO_VERSION < 101200
#error Asio 1.12.0 or newer required.
#endif

#include <chrono>
#include <asio/dispatch.hpp>
#include <asio/io_context.hpp>
#include <asio/executor_work_guard.hpp>
#include <asio/basic_waitable_timer.hpp>
#include <asio/posix/stream_descriptor.hpp>

template <class T>
class TSpan;

namespace tvterm
{

class GrowArray;

class AsyncIOClient
{
public:

    virtual bool onWaitFinish(bool isError, bool isTimeout) = 0;

};

class AsyncIO
{
public:

    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;

private:

    AsyncIOClient &client;

    asio::io_context io;
    asio::executor_work_guard<decltype(io.get_executor())> work;
    asio::posix::stream_descriptor descriptor;
    asio::basic_waitable_timer<Clock> inputWaitTimer;

public:

    // The lifetime of 'aClient' must exceed that of 'this'.
    AsyncIO(AsyncIOClient &aClient, int fd) noexcept;

    void run() noexcept;
    // Thread-safe.
    void stop() noexcept;

    // Thread-safe.
    template <class Func>
    void dispatch(Func &&func) noexcept;

    void waitInput() noexcept;
    void setWaitTimeout(Duration duration) noexcept;
    size_t readInput(TSpan<char> buf) noexcept;
    void writeOutput(GrowArray &&data) noexcept;
};

template <class Func>
inline void AsyncIO::dispatch(Func &&func) noexcept
{
    asio::dispatch(io, std::move(func));
}

} // namespace tvterm

#endif // TVTERM_ASYNCIO_H
