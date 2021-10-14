#ifndef TVTERM_ASYNCSTRAND_H
#define TVTERM_ASYNCSTRAND_H

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

class AsyncStrandClient
{
public:

    virtual void onWaitFinish(int error, bool isTimeout) = 0;

};

class AsyncStrand
{
public:

    using clock = std::chrono::steady_clock;
    using time_point = clock::time_point;

private:

    bool waitingForInput {false};
    AsyncStrandClient &client;

    asio::io_context io;
    asio::executor_work_guard<decltype(io.get_executor())> work;
    asio::posix::stream_descriptor descriptor;
    asio::basic_waitable_timer<clock> inputWaitTimer;

public:

    // The lifetime of 'aClient' must exceed that of 'this'.
    AsyncStrand(AsyncStrandClient &aClient, int fd) noexcept;

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
inline void AsyncStrand::writeOutput(Buffer &&buf) noexcept
{
    if (buf.size())
        asio::async_write(
            descriptor,
            asio::buffer(buf.data(), buf.size()),
            [buf = std::move(buf)] (...) noexcept {}
        );
}

template <class Func>
inline void AsyncStrand::dispatch(Func &&func) noexcept
{
    asio::dispatch(io, std::move(func));
}

} // namespace tvterm

#endif // TVTERM_ASYNCSTRAND_H
