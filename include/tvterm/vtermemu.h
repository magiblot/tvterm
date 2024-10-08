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

    TerminalEmulator &create( TPoint size, Writer &clientDataWriter,
                              Mutex<TerminalSharedState> &sharedState ) noexcept override;

    TSpan<const EnvironmentVar> getCustomEnvironment() noexcept override;

};

class VTermEmulator final : public TerminalEmulator
{
public:

    VTermEmulator(TPoint size, Writer &aClientDataWriter, Mutex<TerminalSharedState> &aSharedState) noexcept;
    ~VTermEmulator();

    void handleEvent(const TerminalEvent &event) noexcept override;
    void flushState() noexcept override;

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
    };

    struct VTerm *vt;
    struct VTermState *state;
    struct VTermScreen *vts;
    Writer &clientDataWriter;
    Mutex<TerminalSharedState> &sharedState;
    GrowArray strFragBuf;
    LineStack linestack;
    LocalState localState;
    bool mouseEnabled {false};
    bool altScreenEnabled {false};

    static const VTermScreenCallbacks callbacks;

    void updateState() noexcept;
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
};

inline TSpan<const VTermScreenCell> VTermEmulator::LineStack::top() const
{
    auto &pair = stack.back();
    return {pair.first.get(), pair.second};
}

} // namespace tvterm

#endif // TVTERM_VTERMEMU_H
