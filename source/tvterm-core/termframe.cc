#include <tvterm/termframe.h>
#include <tvterm/termactiv.h>

#define Uses_TRect
#include <tvision/tv.h>

namespace tvterm
{

BasicTerminalFrame::BasicTerminalFrame(const TRect &bounds) noexcept :
    TFrame(bounds)
{
}

void BasicTerminalFrame::draw()
{
    TFrame::draw();

    if (state & sfDragging)
    {
        TRect r(4, size.y - 1, min(size.x - 4, 14), size.y);
        if (r.a.x < r.b.x && r.a.y < r.b.y)
        {
            TDrawBuffer b;
            char str[256];
            TPoint termSize = {max(size.x - 2, 0), max(size.y - 2, 0)};
            snprintf(str, sizeof(str), " %dx%d ", termSize.x, termSize.y);
            uchar color = mapColor(5);
            ushort width = b.moveStr(0, str, color, r.b.x - r.a.x);
            writeLine(r.a.x, r.a.y, width, r.b.y - r.a.y, b);
        }
    }
}

} // namespace tvterm
