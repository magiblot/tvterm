#ifndef TVTERM_TERMFRAME_H
#define TVTERM_TERMFRAME_H

#define Uses_TFrame
#include <tvision/tv.h>

namespace tvterm
{

class TerminalActivity;

class BasicTerminalFrame : public TFrame
{
    TerminalActivity *term;

    void drawSize() noexcept;

public:

    BasicTerminalFrame(const TRect &bounds) noexcept;

    // If not null, the lifetime of 'term' must exceed that of 'this'.
    void setTerm(TerminalActivity *term) noexcept;

    void draw() override;
};

inline void BasicTerminalFrame::setTerm(TerminalActivity *aTerm) noexcept
{
    term = aTerm;
}

} // namespace tvterm

#endif // TVTERM_TERMFRAME_H
