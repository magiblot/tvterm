#ifndef TVTERM_TERMEMU_H
#define TVTERM_TERMEMU_H

#include <tvterm/pty.h>
#include <tvterm/array.h>
#include <tvterm/mutex.h>
#include <vector>

#define Uses_TDrawSurface
#define Uses_TEvent
#include <tvision/tv.h>

namespace tvterm
{

class TerminalSurface : private TDrawSurface
{
    // A TDrawSurface that can keep track of the areas that were modified.
    // Otherwise, the view displaying this surface would have to copy all of
    // it every time, which doesn't scale well when using big resolutions.

public:

    struct Range { int begin, end; };

    using TDrawSurface::size;

    void resize(TPoint aSize);
    void clearDamage();
    using TDrawSurface::at;
    Range &damageAt(size_t y);
    const Range &damageAt(size_t y) const;
    void setDamage(size_t y, int begin, int end);

private:

    std::vector<Range> rowDamage;
};

inline void TerminalSurface::resize(TPoint aSize)
{
    if (aSize != size)
    {
        TDrawSurface::resize(aSize);
        // The surface's contents are not relevant after the resize.
        clearDamage();
    }
}

inline void TerminalSurface::clearDamage()
{
    rowDamage.resize(0);
    rowDamage.resize(max(0, size.y), {INT_MAX, INT_MIN});
}

inline TerminalSurface::Range &TerminalSurface::damageAt(size_t y)
{
    return rowDamage[y];
}

inline const TerminalSurface::Range &TerminalSurface::damageAt(size_t y) const
{
    return rowDamage[y];
}

inline void TerminalSurface::setDamage(size_t y, int begin, int end)
{
    auto &damage = damageAt(y);
    damage = {
        min(begin, damage.begin),
        max(end, damage.end),
    };
}

struct TerminalSharedState
{
    TerminalSurface surface;
    bool cursorChanged {false};
    TPoint cursorPos {0, 0};
    bool cursorVisible {false};
    bool cursorBlink {false};
    bool titleChanged {false};
    GrowArray title;
};

enum class TerminalEventType
{
    KeyDown,
    Mouse,
    ClientDataRead,
    ViewportResize,
    FocusChange,
};

struct MouseEvent
{
    ushort what;
    MouseEventType mouse;
};

struct ClientDataReadEvent
{
    const char *data;
    size_t size;
};

struct ViewportResizeEvent
{
    int x, y;
};

struct FocusChangeEvent
{
    bool focusEnabled;
};

struct TerminalEvent
{
    TerminalEventType type;
    union
    {
        ::KeyDownEvent keyDown;
        MouseEvent mouse;
        ClientDataReadEvent clientDataRead;
        ViewportResizeEvent viewportResize;
        FocusChangeEvent focusChange;
    };
};

class TerminalEmulator
{
public:

    virtual ~TerminalEmulator() = default;

    virtual void handleEvent(const TerminalEvent &event) noexcept = 0;
    virtual void flushState() noexcept = 0;
};

class Writer
{
public:

    virtual void write(const char *data, size_t size) noexcept = 0;
};

class TerminalEmulatorFactory
{
public:

    // Function returning a new-allocated TerminalEmulator.
    // Both 'clientDataWriter' and 'sharedState' are non-owning references and
    // they exceed the lifetime of the returned TerminalEmulator.
    virtual TerminalEmulator &create( TPoint size,
                                      Writer &clientDataWriter,
                                      Mutex<TerminalSharedState> &sharedState ) noexcept = 0;

    // Returns environment variables that the TerminalEmulator requires the
    // client process to have.
    // The lifetime of the returned value must not be shorter than that of 'this'.
    virtual TSpan<const EnvironmentVar> getCustomEnvironment() noexcept = 0;
};

} // namespace tvterm

#endif // TVTERM_TERMEMU_H
