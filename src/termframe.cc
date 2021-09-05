#include <tvterm/termframe.h>
#include <tvterm/termactiv.h>

#define Uses_TRect
#include <tvision/tv.h>

TerminalFrame::TerminalFrame(const TRect &bounds) :
    TFrame(bounds),
    term(nullptr)
{
}

void TerminalFrame::draw()
{
    TFrame::draw();
    if (term)
        drawSize();
}

void TerminalFrame::drawSize()
// Pre: 'term' != nullptr.
{
    if (state & sfDragging)
    {
        TRect r(4, size.y - 1, min(size.x - 4, 14), size.y);
        if (r.a.x < r.b.x && r.a.y < r.b.y)
        {
            TDrawBuffer b;
            char str[256];
            TPoint termSize = term->getSize();
            snprintf(str, sizeof(str), " %dx%d ", termSize.x, termSize.y);
            uchar color = mapColor(5);
            ushort width = b.moveStr(0, str, color, r.b.x - r.a.x);
            writeLine(r.a.x, r.a.y, width, r.b.y - r.a.y, b);
        }
    }
}
