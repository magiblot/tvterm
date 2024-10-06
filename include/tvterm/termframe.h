#ifndef TVTERM_TERMFRAME_H
#define TVTERM_TERMFRAME_H

#define Uses_TFrame
#include <tvision/tv.h>

namespace tvterm
{

// A TFrame that displays the terminal dimensions while being dragged.
// It assumes that the terminal fills the inner area of the frame.

class BasicTerminalFrame : public TFrame
{
public:

    BasicTerminalFrame(const TRect &bounds) noexcept;

    void draw() override;
};

} // namespace tvterm

#endif // TVTERM_TERMFRAME_H
