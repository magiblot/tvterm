#ifndef TVTERM_TERMADAPT_H
#define TVTERM_TERMADAPT_H

#include <tvterm/array.h>
#include <vector>

#define Uses_TDrawSurface
#include <tvision/tv.h>

struct KeyDownEvent;
struct MouseEventType;

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

class TerminalAdapter
{
protected:

    GrowArray writeBuffer;

public:

    TerminalAdapter() noexcept = default;
    virtual ~TerminalAdapter() {}

    virtual void setSize(TPoint size, TerminalSharedState &sharedState) noexcept = 0;
    virtual void setFocus(bool focus) noexcept = 0;
    virtual void handleKeyDown(const KeyDownEvent &keyDown) noexcept = 0;
    virtual void handleMouse(ushort what, const MouseEventType &mouse) noexcept = 0;
    virtual void receive(TSpan<const char> buf, TerminalSharedState &sharedState) noexcept = 0;
    virtual void flushDamage(TerminalSharedState &sharedState) noexcept = 0;

    GrowArray takeWriteBuffer() noexcept;
};

inline GrowArray TerminalAdapter::takeWriteBuffer() noexcept
{
    return std::move(writeBuffer);
}

} // namespace tvterm

#endif // TVTERM_TERMADAPT_H
