#ifndef TVTERM_TERMACTIV_H
#define TVTERM_TERMACTIV_H

#define Uses_TPoint
#include <tvision/tv.h>

#include <tvterm/asyncio.h>
#include <tvterm/termemu.h>
#include <tvterm/pty.h>
#include <atomic>
#include <memory>
#include <mutex>

namespace tvterm
{

class ThreadPool;

class GrowArrayWriter : public Writer
{
public:

    GrowArray buffer;

    void write(const char *data, size_t size) noexcept override
    {
        buffer.push(data, size);
    }
};

class TerminalActivity final : private AsyncIOClient
{
    friend std::default_delete<TerminalActivity>;

    using Clock = AsyncIO::Clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    PtyProcess pty;
    AsyncIO async;
    GrowArrayWriter clientDataWriter;
    Mutex<TerminalSharedState> sharedState;

    // This must be initialized after 'clientDataWriter' and 'sharedState'.
    TerminalEmulator &terminal;

    std::atomic<bool> updated {false};
    std::atomic<bool> closed {false};

    TerminalActivity( TPoint size,
                      TerminalEmulatorFactory &,
                      PtyDescriptor, ThreadPool & ) noexcept;
    ~TerminalActivity();

private:

    // Stuff acessed only from child thread.

    enum { bufSize = 4096 };
    enum { maxConsecutiveEOF = 5 };
    enum { maxReadTimeMs = 20, inputWaitStepMs = 5 };
    enum WaitState : uint8_t { wsInitial, wsRead, wsFlush, wsEOF };

    WaitState waitState {wsRead};
    // case wsRead:
    TimePoint readTimeout;

    int consecutiveEOF {0};

    bool viewportSizeChanged {false};
    TPoint viewportSize;

    bool onWaitFinish(bool, bool) noexcept override;
    void checkSize() noexcept;

public:

    // The lifetime of 'threadPool' must exceed that of the returned TerminalActivity.
    static TerminalActivity *create( TPoint size,
                                     TerminalEmulatorFactory &createTerminal,
                                     void (&onError)(const char *reason),
                                     ThreadPool &threadPool ) noexcept;
    // Takes ownership over 'this'.
    void destroy() noexcept;

    bool hasChanged() noexcept;
    bool isClosed() const noexcept;
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
    return closed;
}

template <class Func>
inline auto TerminalActivity::getState(Func &&func)
{
    return sharedState.lock(static_cast<Func &&>(func));
}

} // namespace tvterm

#endif // TVTERM_TERMACTIV_H
