#ifndef TVTERM_TERMACTIV_H
#define TVTERM_TERMACTIV_H

#define Uses_TPoint
#include <tvision/tv.h>

#include <tvterm/asyncio.h>
#include <tvterm/termadapt.h>
#include <tvterm/pty.h>
#include <atomic>
#include <memory>
#include <mutex>

namespace tvterm
{

class ThreadPool;

class TerminalActivity final : private AsyncIOClient
{
    friend std::default_delete<TerminalActivity>;

    using clock = AsyncIO::clock;
    using time_point = AsyncIO::time_point;

    enum { bufSize = 4096 };
    enum { maxConsecutiveEOF = 5 };
    enum WaitState : uint8_t { wsReady, wsRead, wsFlush, wsEOF };
    enum { maxReadTimeMs = 20, inputWaitStepMs = 5 };

    PtyProcess pty;
    AsyncIO async;
    GrowArray outputBuffer;

    WaitState waitState {wsRead};
    // case wsReady:
    int consecutiveEOF {0};
    // case wsRead:
    time_point readTimeout;

    bool viewSizeChanged {false};
    TPoint viewSize;

    std::atomic<bool> updated {false};
    Mutex<TerminalSharedState> sharedState;

    TerminalAdapter &terminal;

    TerminalActivity( TPoint size,
                      TerminalAdapterFactory &,
                      PtyDescriptor, ThreadPool & ) noexcept;
    ~TerminalActivity();

    void onWaitFinish(int, bool) noexcept override;
    void advanceWaitState(int, bool) noexcept;
    void checkSize() noexcept;

public:

    // The lifetime of 'threadPool' must exceed that of the returned TerminalActivity.
    static TerminalActivity *create( TPoint size,
                                     TerminalAdapterFactory &createTerminal,
                                     void (&childActions)(),
                                     void (&onError)(const char *reason),
                                     ThreadPool &threadPool ) noexcept;
    // Takes ownership over 'this'.
    void destroy() noexcept;

    bool hasChanged() noexcept;
    bool isClosed() const noexcept;
    TPoint getSize() const noexcept;
    void sendResize(TPoint aSize) noexcept;
    void sendFocus(bool focus) noexcept;
    void sendKeyDown(const KeyDownEvent &keyDown) noexcept;
    void sendMouse(ushort what, const MouseEventType &mouse) noexcept;

    template <class Func>
    // This method locks a mutex, so reentrance will lead to a deadlock.
    // * 'func' takes a 'TerminalSharedState &' by parameter.
    auto getState(Func &&func);

};

inline bool TerminalActivity::hasChanged() noexcept
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
    return sharedState.lock(static_cast<Func &&>(func));
}

} // namespace tvterm

#endif // TVTERM_TERMACTIV_H
