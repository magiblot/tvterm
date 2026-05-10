#ifndef TVTERM_CONSTS_H
#define TVTERM_CONSTS_H

namespace tvterm
{

struct TVTermConstants
{
    ushort cmCheckTerminalUpdates;
    ushort cmTerminalUpdated;
    ushort cmCopySelection;
    ushort cmCancelSelection;
    // Focused commands
    ushort cmGrabInput;
    ushort cmReleaseInput;
    ushort cmStartSelection;
    // Help contexts
    ushort hcInputGrabbed;
    ushort hcSelecting;

    // Commands that are active when a terminal window is focused.
    TSpan<const ushort> focusedCmds() const
    {
        // NOTE: This requires 'focused commands' to be contiguous in this
        // struct.
        return {&cmGrabInput, size_t(&cmStartSelection + 1 - &cmGrabInput)};
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
