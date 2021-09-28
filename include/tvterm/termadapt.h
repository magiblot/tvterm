#ifndef TVTERM_TERMADAPT_H
#define TVTERM_TERMADAPT_H

#include <tvterm/mutex.h>
#include <vector>

#define Uses_TDrawSurface
#include <tvision/tv.h>

struct KeyDownEvent;
struct MouseEventType;

class TerminalSurface : private TDrawSurface
{
    // A TDrawSurface that can keep track of the areas that were modified.
    // Otherwise, the view displaying this surface would have to copy all of
    // it, which doesn't scale well when using big resolutions.

public:

    struct Range { int begin, end; };

    using TDrawSurface::size;

    void resize(TPoint aSize);
    void clearAllDamage();
    void damageAll();
    using TDrawSurface::at;
    Range &damageAt(size_t y);
    void setDamage(size_t y, int begin, int end);
    void clearDamage(size_t y, int begin, int end);

private:

    std::vector<Range> rowDamage;
};

inline void TerminalSurface::resize(TPoint aSize)
{
    if (aSize != size)
    {
        TDrawSurface::resize(aSize);
        // The surface's contents are irrelevant after the resize.
        clearAllDamage();
    }
}

inline void TerminalSurface::clearAllDamage()
{
    rowDamage.resize(0);
    rowDamage.resize(max(0, size.y), {INT_MAX, INT_MIN});
}

inline void TerminalSurface::damageAll()
{
    rowDamage.resize(0);
    rowDamage.resize(max(0, size.y), {INT_MIN, INT_MAX});
}

inline TerminalSurface::Range &TerminalSurface::damageAt(size_t y)
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

inline void TerminalSurface::clearDamage(size_t y, int begin, int end)
{
    auto &damage = damageAt(y);
    if (begin <= damage.begin)
        damage.begin = max(damage.begin, end);
    if (damage.end <= end)
        damage.end = min(damage.end, begin);
}

struct TerminalReceivedState
{
    TerminalSurface surface;
    bool cursorChanged {false};
    TPoint cursorPos {0, 0};
    bool cursorVisible {false};
    bool cursorBlink {false};
    bool titleChanged {false};
    std::vector<char> title;
};

class TerminalAdapter
{
    TMutex<TerminalReceivedState> mState;

protected:

    std::vector<char> writeBuffer;

public:

    TerminalAdapter(TPoint size) noexcept;
    virtual ~TerminalAdapter() {}

    virtual void (&getChildActions() noexcept)() = 0;
    virtual void setSize(TPoint size) noexcept = 0;
    virtual void handleKeyDown(const KeyDownEvent &keyDown) noexcept = 0;
    virtual void handleMouse(ushort what, const MouseEventType &mouse) noexcept = 0;
    virtual void receive(TSpan<const char> buf) noexcept = 0;
    virtual void flushDamage() noexcept = 0;

    std::vector<char> takeWriteBuffer() noexcept;
    template <class Func>
    // This method locks a mutex, so reentrance will lead to a deadlock.
    // * 'func' takes a 'TerminalReceivedState &' by parameter.
    auto getState(Func &&func);

};

inline TerminalAdapter::TerminalAdapter(TPoint size) noexcept
{
    mState.get().surface.resize(size);
}

inline std::vector<char> TerminalAdapter::takeWriteBuffer() noexcept
{
    return std::move(writeBuffer);
}

template <class Func>
inline auto TerminalAdapter::getState(Func &&func)
{
    return mState.lock(std::move(func));
}

#endif // TVTERM_TERMADAPT_H
