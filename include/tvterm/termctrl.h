#ifndef TVTERM_TERMCTRL_H
#define TVTERM_TERMCTRL_H

#define Uses_TPoint
#include <tvision/tv.h>

#include <tvterm/termemu.h>
#include <tvterm/pty.h>
#include <atomic>
#include <memory>

namespace tvterm
{

class TerminalController
{
public:

    // Returns a new-allocated TerminalController.
    // On error, invokes the 'onError' callback and returns null.
    static TerminalController *create( TPoint size,
                                     TerminalEmulatorFactory &terminalEmulatorFactory,
                                     void (&onError)(const char *reason) ) noexcept;
    // Takes ownership over 'this'.
    void shutDown() noexcept;

    void sendEvent(const TerminalEvent &event) noexcept;

    bool stateHasBeenUpdated() noexcept;
    bool clientIsDisconnected() noexcept;

    template <class Func>
    // This method locks a mutex, so reentrance will lead to a deadlock.
    // * 'func' takes a 'TerminalState &' by parameter.
    auto lockState(Func &&func);

private:

    struct TerminalEventLoop;

    PtyMaster ptyMaster;
    Mutex<TerminalState> terminalState;
    TerminalEventLoop &eventLoop;
    TerminalEmulator &terminalEmulator;

    std::atomic<bool> updated {false};
    std::atomic<bool> disconnected {false};

    std::shared_ptr<TerminalController> selfOwningPtr;

    TerminalController(TPoint, TerminalEmulatorFactory &, PtyDescriptor) noexcept;
    ~TerminalController();
};

inline bool TerminalController::stateHasBeenUpdated() noexcept
{
    return updated.exchange(false) == true;
}

inline bool TerminalController::clientIsDisconnected() noexcept
{
    return disconnected;
}

template <class Func>
inline auto TerminalController::lockState(Func &&func)
{
    return terminalState.lock([&] (auto &state) {
        return func(state);
    });
}

} // namespace tvterm

#endif // TVTERM_TERMCTRL_H
