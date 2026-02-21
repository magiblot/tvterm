#ifndef TVTERM_CONSTS_H
#define TVTERM_CONSTS_H

namespace tvterm
{

struct TVTermConstants
{
    ushort cmCheckTerminalUpdates;
    ushort cmTerminalUpdated;
    // Focused commands (enabled/disabled on window focus)
    ushort cmGrabInput;
    ushort cmReleaseInput;
    ushort cmCopySelection;
    // Help contexts
    ushort hcInputGrabbed;

    TSpan<const ushort> focusedCmds() const
    {
        return {&cmGrabInput, size_t(&cmCopySelection + 1 - &cmGrabInput)};
    }
};

class TerminalView;
struct TerminalState;

struct TerminalUpdatedMsg
{
    TerminalView &view;
    TerminalState &state;
};

} // namespace tvterm

#endif // TVTERM_CONSTS_H
