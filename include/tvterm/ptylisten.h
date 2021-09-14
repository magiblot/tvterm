#ifndef TVTERM_PTYLISTEN_H
#define TVTERM_PTYLISTEN_H

#define Uses_TPoint
#include <tvision/tv.h>

#include <tvterm/refcnt.h>
#include <tvterm/pty.h>
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
    static constexpr auto inputWaitStep = std::chrono::milliseconds(5);

    PtyProcess pty;

    asio::io_context::strand strand;
    asio::posix::stream_descriptor descriptor;
    asio::basic_waitable_timer<clock> inputWaitTimer;
    bool waitingForInput {false};

    enum WaitState : uint8_t { wsReady, wsRead, wsFlush, wsEOF };
    WaitState waitState {wsRead};

    // waitState == wsRead
    time_point readTimeout;

    int consecutiveEOF {0};

    TPoint viewSize;
    TRc<bool> alive {TRc<bool>::make(true)};
    std::atomic<bool> updated {false};

    PTYListener(TPoint size, PtyDescriptor, TerminalAdapter &aTerminal, asio::io_context &io) noexcept;
    ~PTYListener();

public:

    TerminalAdapter &terminal;

    static PTYListener &create(TPoint size, PtyDescriptor ptyDescriptor, TerminalAdapter &aTerminal, asio::io_context &io) noexcept;

    void destroy();

    bool checkChanges();
    bool isClosed() const;
    TPoint getSize() const noexcept;
    void changeSize(TPoint aSize) noexcept;

    template <class Func>
    void run(Func &&func);

    void writeOutput(std::vector<char> &&buf) noexcept;

private:

    bool streamIsNotEmpty() noexcept;
    void waitInput() noexcept;
    void waitInputUntil(time_point timeout) noexcept;
    void advanceWaitState(int, bool) noexcept;
    void checkSize() noexcept;

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
    return waitState == wsEOF;
}

inline TPoint PTYListener::getSize() const noexcept
{
    return pty.getSize();
}

inline void PTYListener::changeSize(TPoint aSize) noexcept
{
    viewSize = aSize;
    checkSize();
}

#endif // TVTERM_PTYLISTEN_H
