#ifndef TVTERM_TERMACTIV_H
#define TVTERM_TERMACTIV_H

#define Uses_TPoint
#include <tvision/tv.h>

#include <tvterm/asyncstrand.h>
#include <tvterm/termadapt.h>
#include <tvterm/pty.h>
#include <atomic>
#include <memory>

namespace tvterm
{

class TerminalActivity final : private AsyncStrandClient
{
    friend std::default_delete<TerminalActivity>;

    using clock = AsyncStrand::clock;
    using time_point = AsyncStrand::time_point;

    enum { bufSize = 4096 };
    enum { maxConsecutiveEOF = 5 };
    enum WaitState : uint8_t { wsReady, wsRead, wsFlush, wsEOF };
    static constexpr auto maxReadTime = std::chrono::milliseconds(20);
    static constexpr auto inputWaitStep = std::chrono::milliseconds(5);

    PtyProcess pty;
    TerminalAdapter &terminal;
    AsyncStrand async;

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

    // Takes ownership over 'terminal'. The lifetime of 'io' must not exceed
    // that of the result.
    static TerminalActivity *create( TPoint size, TerminalAdapter &terminal,
                                     asio::io_context &io,
                                     void (&onError)(const char *reason) ) noexcept;
    // Takes ownership over 'this'.
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

} // namespace tvterm

#endif // TVTERM_TERMACTIV_H
