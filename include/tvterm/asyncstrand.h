#ifndef TVTERM_ASYNCSTRAND_H
#define TVTERM_ASYNCSTRAND_H

#include <tvterm/refcnt.h>
#include <tvterm/io.h>
#include <chrono>
#include <asio/write.hpp>
#include <asio/buffer.hpp>
#include <asio/dispatch.hpp>
#include <asio/bind_executor.hpp>
#include <asio/io_context_strand.hpp>
#include <asio/basic_waitable_timer.hpp>
#include <asio/posix/stream_descriptor.hpp>

template <class T>
class TSpan;

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

    AsyncStrand(asio::io_context &io, int fd) noexcept;
    ~AsyncStrand();

    void setClient(AsyncStrandClient *aClient) noexcept;

    template <class Func>
    void dispatch(Func &&func) noexcept;

    void waitInput() noexcept;
    void waitInputUntil(time_point timeout) noexcept;

    bool canReadInput() noexcept;
    size_t readInput(TSpan<char> buf) noexcept;
    template <class Buffer>
    void writeOutput(Buffer &&buf) noexcept;

private:

    asio::io_context::strand strand;
    asio::posix::stream_descriptor descriptor;
    asio::basic_waitable_timer<clock> inputWaitTimer;
    bool waitingForInput {false};
    TRc<bool> alive {TRc<bool>::make(true)};
    AsyncStrandClient *client {nullptr};
};

inline AsyncStrand::AsyncStrand(asio::io_context &io, int fd) noexcept :
    strand(io),
    descriptor(io, fd),
    inputWaitTimer(io)
{
}

inline AsyncStrand::~AsyncStrand()
{
    *alive = false;
}

inline void AsyncStrand::setClient(AsyncStrandClient *aClient) noexcept
{
    client = aClient;
}

template <class Buffer>
inline void AsyncStrand::writeOutput(Buffer &&buf) noexcept
{
    if (buf.size())
        asio::async_write(
            descriptor,
            asio::buffer(buf.data(), buf.size()),
            asio::bind_executor(strand, [buf = std::move(buf)] (...) noexcept {})
        );
}

template <class Func>
inline void AsyncStrand::dispatch(Func &&func) noexcept
{
    asio::dispatch(strand, std::move(func));
}

#endif // TVTERM_ASYNCSTRAND_H
