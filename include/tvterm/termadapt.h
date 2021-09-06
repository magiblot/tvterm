#ifndef TVTERM_TERMADAPT_H
#define TVTERM_TERMADAPT_H

#include <tvterm/mutex.h>

#define Uses_TDrawSurface
#include <tvision/tv.h>


struct KeyDownEvent;
struct MouseEventType;

struct Range { int begin, end; };

class TerminalSurface : public TDrawSurface
{
    // A TDrawSurface that can keep track of the areas that were modified.
    // Otherwise, the view displaying this surface would have to copy all of
    // it, which doesn't scale well when using big resolutions.

    std::vector<Range> rowDamage;

public:

    void resize(TPoint aSize);
    void clearDamage();
    void damageAll();
    Range &damageAt(size_t y);
};

inline void TerminalSurface::resize(TPoint aSize)
{
    if (aSize != size)
    {
        TDrawSurface::resize(aSize);
        // The surface's contents are meaningless after the resize.
        clearDamage();
    }
}

inline void TerminalSurface::clearDamage()
{
    rowDamage.resize(0);
    rowDamage.resize(max(0, size.y), {INT_MAX, INT_MIN});
}

inline void TerminalSurface::damageAll()
{
    rowDamage.resize(0);
    rowDamage.resize(max(0, size.y), {INT_MIN, INT_MAX});
}

inline Range &TerminalSurface::damageAt(size_t y)
{
    return rowDamage[y];
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

    virtual void setSize(TPoint size) noexcept = 0;

public:

    TerminalAdapter(TPoint size) noexcept;
    virtual ~TerminalAdapter() {}

    virtual void damageAll() noexcept = 0;
    virtual void handleKeyDown(const KeyDownEvent &keyDown) noexcept = 0;
    virtual void handleMouse(ushort what, const MouseEventType &mouse) noexcept = 0;
    virtual void receive(TSpan<const char> buf) noexcept = 0;
    virtual void flushDamage() noexcept = 0;

    std::vector<char> takeWriteBuffer() noexcept;
    template <class Func>
    auto getState(Func &&func);
    void changeSize(TPoint size) noexcept;

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

inline void TerminalAdapter::changeSize(TPoint size) noexcept
{
    getState([&] (auto &state) { state.surface.resize(size); });
    // The implementation may clear the screen after a resize. To ensure the
    // the surface is not left blank, we redraw it again first.
    damageAll();
    setSize(size);
}

#endif // TVTERM_TERMADAPT_H
