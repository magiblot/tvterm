#ifndef TVTERM_VTERMFRAME_H
#define TVTERM_VTERMFRAME_H

#define Uses_TFrame
#include <tvision/tv.h>

struct TVTermView;

struct TVTermFrame : public TFrame
{

    TVTermView *term;

    TVTermFrame(const TRect &bounds);

    void setTerm(TVTermView *term);

    void draw() override;
    void drawSize();

};

inline void TVTermFrame::setTerm(TVTermView *t)
{
    term = t;
}

#endif // TVTERM_VTERMFRAME_H
