#ifndef TVTERM_VTERMADAPT_H
#define TVTERM_VTERMADAPT_H

#include <tvterm/termadapt.h>
#include <utility>
#include <vector>
#include <memory>

#include <vterm.h>

namespace tvterm
{

class VTermAdapter final : public TerminalAdapter
{
    struct LineStack
    {
        enum { maxSize = 10000 };

        std::vector<std::pair<std::unique_ptr<const VTermScreenCell[]>, size_t>> stack;
        void push(size_t cols, const VTermScreenCell *cells);
        bool pop(const VTermAdapter &vterm, size_t cols, VTermScreenCell *cells);
        TSpan<const VTermScreenCell> top() const;
    };

    struct VTerm *vt;
    struct VTermState *state;
    struct VTermScreen *vts;
    TerminalSharedState *sharedState;
    ByteArray strFragBuf;
    LineStack linestack;
    bool mouseEnabled {false};
    bool altScreenEnabled {false};

    static const VTermScreenCallbacks callbacks;

    void writeOutput(const char *data, size_t size);
    int damage(VTermRect rect);
    int moverect(VTermRect dest, VTermRect src);
    int movecursor(VTermPos pos, VTermPos oldpos, int visible);
    int settermprop(VTermProp prop, VTermValue *val);
    int bell();
    int resize(int rows, int cols);
    int sb_pushline(int cols, const VTermScreenCell *cells);
    int sb_popline(int cols, VTermScreenCell *cells);

    VTermScreenCell getDefaultCell() const;
    TPoint getSize() noexcept;

    void receive(TSpan<const char> buf, TerminalSharedState &aSharedState) noexcept override;
    void flushDamage(TerminalSharedState &aSharedState) noexcept override;
    void setSize(TPoint size, TerminalSharedState &aSharedState) noexcept override;
    void setFocus(bool focus) noexcept override;
    void handleKeyDown(const KeyDownEvent &keyDown) noexcept override;
    void handleMouse(ushort what, const MouseEventType &mouse) noexcept override;

public:

    VTermAdapter(TPoint size, TerminalSharedState &aSharedState) noexcept;
    ~VTermAdapter();

    static TerminalAdapter &create(TPoint size, TerminalSharedState &aSharedState) noexcept;
    static void childActions() noexcept;
};

inline TerminalAdapter &VTermAdapter::create(TPoint size, TerminalSharedState &aSharedState) noexcept
{
    return *new VTermAdapter(size, aSharedState);
}

inline TSpan<const VTermScreenCell> VTermAdapter::LineStack::top() const
{
    auto &pair = stack.back();
    return {pair.first.get(), pair.second};
}

} // namespace tvterm

#endif // TVTERM_VTERMADAPT_H
