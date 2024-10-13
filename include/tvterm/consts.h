#ifndef TVTERM_CONSTS_H
#define TVTERM_CONSTS_H

namespace tvterm
{

struct TVTermConstants
{
    ushort cmCheckTerminalUpdates;
    ushort cmTerminalUpdated;
    // Focused commands
    ushort cmGrabInput;
    ushort cmReleaseInput;
    // Help contexts
    ushort hcInputGrabbed;

    TSpan<const ushort> focusedCmds() const
    {
        return {&cmGrabInput, size_t(&cmReleaseInput + 1 - &cmGrabInput)};
    }
};

struct TerminalView;
struct TerminalState;

struct TerminalUpdatedMsg
{
    TerminalView &view;
    TerminalState &state;
};

} // namespace tvterm

#endif // TVTERM_CONSTS_H
