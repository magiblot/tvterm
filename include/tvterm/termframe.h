#ifndef TVTERM_TERMFRAME_H
#define TVTERM_TERMFRAME_H

#define Uses_TFrame
#include <tvision/tv.h>

class TerminalActivity;

class TerminalFrame : public TFrame
{
    TerminalActivity *term;

    void drawSize();

public:

    TerminalFrame(const TRect &bounds);

    // If not null, the lifetime of 'term' must exceed that of 'this'.
    void setTerm(TerminalActivity *term);

    void draw() override;
};

inline void TerminalFrame::setTerm(TerminalActivity *aTerm)
{
    term = aTerm;
}

#endif // TVTERM_TERMFRAME_H
