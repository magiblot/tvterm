#ifndef TVTERM_PTYLISTEN_H
#define TVTERM_PTYLISTEN_H

#include <tvision/tv.h>

#include <tvterm/refcnt.h>
#include <tvterm/io.h>
#include <chrono>
#include <atomic>
#include <memory>

class TerminalAdapter;

class PTYListener
{
    friend std::default_delete<PTYListener>;
    using clock = std::chrono::steady_clock;
    using time_point = clock::time_point;

    enum { bufSize = 4096 };
    enum { maxConsecutiveEOF = 5 };
    static constexpr auto maxReadTime = std::chrono::milliseconds(20);
    static constexpr auto readWaitStep = std::chrono::milliseconds(5);

    asio::io_context::strand strand;
    asio::posix::stream_descriptor descriptor;
    asio::basic_waitable_timer<clock> timer;
    int consecutiveEOF {0};
    bool reachedEOF {false};
    TRc<bool> alive {TRc<bool>::make(true)};
    std::atomic<bool> updated {false};

    static thread_local char staticBuf alignas(4096) [bufSize];

    PTYListener(TerminalAdapter &aTerminal, asio::io_context &io, int fd) noexcept;
    ~PTYListener();

public:

    TerminalAdapter &terminal;

    static PTYListener &create(TerminalAdapter &aTerminal, asio::io_context &io, int fd) noexcept;

    void destroy();

    bool checkChanges();
    bool isClosed() const;

    template <class Func>
    void run(Func &&func);

    void writeOutput(std::vector<char> &&buf);

private:

    bool streamNotEmpty();
    void waitInput();
    void handleReadableInput(const asio::error_code &error);
    void doReadCycle(TSpan<const char> buf, time_point limit);
    template <class Func>
    void readInputUntil(time_point timeout, Func &&);

};

template <class Func>
inline void PTYListener::run(Func &&func)
{
    asio::dispatch(strand, std::move(func));
}

inline bool PTYListener::checkChanges()
{
    return updated.exchange(false) == true;
}

inline bool PTYListener::isClosed() const
{
    return reachedEOF;
}

#endif // TVTERM_PTYLISTEN_H
