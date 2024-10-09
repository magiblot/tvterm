#ifndef TVTERM_VTERMEMU_H
#define TVTERM_VTERMEMU_H

#include <tvterm/termemu.h>
#include <utility>
#include <memory>

#include <vterm.h>

namespace tvterm
{

class VTermEmulatorFactory final : public TerminalEmulatorFactory
{
public:

    TerminalEmulator &create(TPoint size, Writer &clientDataWriter) noexcept override;
    TSpan<const EnvironmentVar> getCustomEnvironment() noexcept override;

};

class VTermEmulator final : public TerminalEmulator
{
public:

    VTermEmulator(TPoint size, Writer &aClientDataWriter) noexcept;
    ~VTermEmulator();

    void handleEvent(const TerminalEvent &event) noexcept override;
    void updateState(TerminalState &state) noexcept override;

private:

    struct LineStack
    {
        enum { maxSize = 10000 };

        std::vector<std::pair<std::unique_ptr<const VTermScreenCell[]>, size_t>> stack;
        void push(size_t cols, const VTermScreenCell *cells);
        bool pop(const VTermEmulator &vterm, size_t cols, VTermScreenCell *cells);
        TSpan<const VTermScreenCell> top() const;
    };

    struct LocalState
    {
        bool cursorChanged {false};
        TPoint cursorPos {0, 0};
        bool cursorVisible {false};
        bool cursorBlink {false};

        bool titleChanged {false};
        GrowArray title;

        bool mouseEnabled {false};
        bool altScreenEnabled {false};
    };

    struct VTerm *vt;
    struct VTermState *vtState;
    struct VTermScreen *vtScreen;
    Writer &clientDataWriter;
    std::vector<TerminalSurface::RowDamage> damageByRow;
    GrowArray strFragBuf;
    LineStack linestack;
    LocalState localState;

    static const VTermScreenCallbacks callbacks;

    TPoint getSize() noexcept;
    void setSize(TPoint size) noexcept;
    void drawDamagedArea(TerminalSurface &surface) noexcept;

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
};

inline TSpan<const VTermScreenCell> VTermEmulator::LineStack::top() const
{
    auto &pair = stack.back();
    return {pair.first.get(), pair.second};
}

} // namespace tvterm

#endif // TVTERM_VTERMEMU_H
