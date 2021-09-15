#ifndef TVTERM_TERMACTIV_H
#define TVTERM_TERMACTIV_H

#define Uses_TPoint
#include <tvision/tv.h>

#include <tvterm/termadapt.h>
#include <tvterm/refcnt.h>
#include <tvterm/pty.h>
#include <tvterm/io.h>
#include <chrono>
#include <atomic>
#include <memory>

class TerminalAdapter;

class AsyncIOStrand
{
protected:

    using clock = std::chrono::steady_clock;
    using time_point = clock::time_point;

    AsyncIOStrand(asio::io_context &io, int fd) noexcept;
    ~AsyncIOStrand();

    bool canReadInput() noexcept;
    void waitInput() noexcept;
    void waitInputUntil(time_point timeout) noexcept;
    template <class Buffer>
    void writeOutput(Buffer &&buf) noexcept;
    size_t readInput(TSpan<char> buf) noexcept;
    template <class Func>
    void dispatch(Func &&func) noexcept;

    virtual void onWaitFinish(int error, bool isTimeout) = 0;

private:

    asio::io_context::strand strand;
    asio::posix::stream_descriptor descriptor;
    asio::basic_waitable_timer<clock> inputWaitTimer;
    bool waitingForInput {false};
    TRc<bool> alive {TRc<bool>::make(true)};
};

class TerminalActivity final : protected AsyncIOStrand
{
    friend std::default_delete<TerminalActivity>;

    enum { bufSize = 4096 };
    enum { maxConsecutiveEOF = 5 };
    enum WaitState : uint8_t { wsReady, wsRead, wsFlush, wsEOF };
    static constexpr auto maxReadTime = std::chrono::milliseconds(20);
    static constexpr auto inputWaitStep = std::chrono::milliseconds(5);

    PtyProcess pty;
    TerminalAdapter &terminal;

    WaitState waitState {wsRead};
    // case wsReady:
    int consecutiveEOF {0};
    // case wsRead:
    time_point readTimeout;

    bool viewSizeChanged {false};
    TPoint viewSize;

    std::atomic<bool> updated {false};

    TerminalActivity(PtyDescriptor, TerminalAdapter &aTerminal, asio::io_context &io) noexcept;
    ~TerminalActivity();

    void onWaitFinish(int, bool) noexcept override;
    void advanceWaitState(int, bool) noexcept;
    void checkSize() noexcept;

public:

    static TerminalActivity *create( TPoint size, TerminalAdapter &terminal,
                                asio::io_context &io,
                                void (&onError)(const char *reason) ) noexcept;

    void destroy() noexcept;

    bool checkChanges() noexcept;
    bool isClosed() const noexcept;
    TPoint getSize() const noexcept;
    void changeSize(TPoint aSize) noexcept;
    void sendKeyDown(const KeyDownEvent &keyDown) noexcept;
    void sendMouse(ushort what, const MouseEventType &mouse) noexcept;

    template <class Func>
    // This method locks a mutex, so reentrance will lead to a deadlock.
    // * 'func' takes a 'TerminalReceivedState &' by parameter.
    auto getState(Func &&func);

};

inline bool TerminalActivity::checkChanges() noexcept
{
    return updated.exchange(false) == true;
}

inline bool TerminalActivity::isClosed() const noexcept
{
    return waitState == wsEOF;
}

inline TPoint TerminalActivity::getSize() const noexcept
{
    return pty.getSize();
}

template <class Func>
inline auto TerminalActivity::getState(Func &&func)
{
    return terminal.getState(std::move(func));
}

#endif // TVTERM_TERMACTIV_H
