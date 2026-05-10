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

struct RowRange
{
    // The range is valid when 'start < end'.
    // Otherwise, we use {INT_MAX, INT_MIN}, which makes it simple to adjust
    // ranges with min()/max().
    int start {INT_MAX};
    int end {INT_MIN};

    RowRange() = default;
    RowRange(int start, int end) noexcept : start(start), end(end) {}

    int size() const noexcept;
    RowRange intersect(RowRange other) const noexcept;

    friend bool operator!=(const RowRange &a, const RowRange &b) noexcept;
};

inline int RowRange::size() const noexcept
{
    if (start < end)
        return end - start;
    return 0;
}

inline RowRange RowRange::intersect(RowRange other) const noexcept
{
    return {
        max(start, other.start),
        min(end, other.end),
    };
}

inline bool operator!=(const RowRange &a, const RowRange &b) noexcept
{
    return a.start != b.start || a.end != b.end;
}

struct SelectionAnchor
{
    // In absolute coordinates: y = 0 is the oldest scrollback line.
    int x, y;

    SelectionAnchor() = default; // Trivial constructor required by TerminalEvent.
    SelectionAnchor(int x, int y) noexcept : x(x), y(y) {}
    SelectionAnchor(TPoint p) noexcept : x(p.x), y(p.y) {}

    operator TPoint() const noexcept { return {x, y}; }

    friend bool operator<(const SelectionAnchor &a, const SelectionAnchor &b) noexcept;
    friend bool operator>(const SelectionAnchor &a, const SelectionAnchor &b) noexcept;
};

inline bool operator<(const SelectionAnchor &a, const SelectionAnchor &b) noexcept
{
    return a.y < b.y || (a.y == b.y && a.x < b.x);
}

inline bool operator>(const SelectionAnchor &a, const SelectionAnchor &b) noexcept
{
    return b < a;
}

struct SelectionRange
{
    // Both 'start < end' and 'start > end' are valid.
    // When 'start == end', there is no selection.
    // Selection goes from {start.x, start.y} to {end.x - 1, end.y}
    SelectionAnchor start;
    SelectionAnchor end;

    bool empty() const noexcept;
    RowRange intersectRow(int y, int cols) const noexcept;
};

inline bool SelectionRange::empty() const noexcept
{
    return start == end;
}

inline RowRange SelectionRange::intersectRow(int y, int cols) const noexcept
{
    SelectionRange normalized = {
        min(start, end),
        max(start, end),
    };

    if (empty() || y < normalized.start.y || y > normalized.end.y)
        return {INT_MAX, INT_MIN};

    return {
        (y == normalized.start.y) ? max(normalized.start.x, 0) : 0,
        (y == normalized.end.y) ? min(normalized.end.x, cols) : cols,
    };
}

// A TDrawSurface that can keep track of the areas that were modified.
// Otherwise, the view displaying this surface would have to copy all of
// it every time, which doesn't scale well when using big resolutions.

class TerminalSurface : private TDrawSurface
{
public:

    using TDrawSurface::size;

    void resize(TPoint aSize);
    void clearDamage();
    using TDrawSurface::at;
    RowRange &damageAtRow(size_t y);
    const RowRange &damageAtRow(size_t y) const;
    void addDamageAtRow(size_t y, int begin, int end);

private:

    std::vector<RowRange> damageByRow;
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
    damageByRow.resize(0);
    damageByRow.resize(max(0, size.y));
}

inline RowRange &TerminalSurface::damageAtRow(size_t y)
{
    return damageByRow[y];
}

inline const RowRange &TerminalSurface::damageAtRow(size_t y) const
{
    return damageByRow[y];
}

inline void TerminalSurface::addDamageAtRow(size_t y, int begin, int end)
{
    auto &damage = damageAtRow(y);
    damage = {
        min(begin, damage.start),
        max(end, damage.end),
    };
}

// Struct that allows the terminal state to be reported from the TerminalEmulator
// to the TerminalController (and from there to the TerminalView, etc.)

struct TerminalState
{
    TerminalSurface surface;

    bool cursorChanged {false};
    TPoint cursorPos {0, 0};
    bool cursorVisible {false};
    bool cursorBlink {false};

    bool titleChanged {false};
    GrowArray title;

    bool scrollbackChanged {false};
    int scrollbackOffset {0}; // 0 (oldest) to 'scrollbackLimit' (no scrollback shown)
    int scrollbackLimit {0};
    bool scrollbackEnabled {true};

    bool mouseEnabled {false};
};

enum class TerminalEventType
{
    KeyDown,
    Mouse,
    ClientDataRead,
    ViewportResize,
    FocusChange,
    ScrollBackOffsetChange,
    CopySelection,
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

struct ScrollBackOffsetChangeEvent
{
    int offset;
};

struct CopySelectionEvent
{
    SelectionRange selection;
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
        ScrollBackOffsetChangeEvent scrollBackOffsetChange;
        CopySelectionEvent copySelection;
    };
};

class TerminalEmulator
{
public:

    virtual ~TerminalEmulator() = default;

    // Gets called whenever the user interacts with the terminal or the
    // client process sends data.
    virtual void handleEvent(const TerminalEvent &event) noexcept = 0;
    // Gets called shortly after 'handleEvent()', so that any changes that
    // may have occurred can be reported via the TerminalState.
    virtual void updateState(TerminalState &state) noexcept = 0;
};

class Writer
{
public:

    virtual void write(TSpan<const char> data) noexcept = 0;
};

class TerminalEmulatorFactory
{
public:

    // Function returning a new-allocated TerminalEmulator.
    // 'clientDataWriter' is a non-owning reference and exceeds the lifetime
    // of the returned TerminalEmulator.
    virtual TerminalEmulator &create( TPoint size,
                                      Writer &clientDataWriter ) noexcept = 0;

    // Returns environment variables that the TerminalEmulator requires the
    // client process to have.
    // The lifetime of the returned value must not be shorter than that of 'this'.
    virtual TSpan<const EnvironmentVar> getCustomEnvironment() noexcept = 0;
};

} // namespace tvterm

#endif // TVTERM_TERMEMU_H
