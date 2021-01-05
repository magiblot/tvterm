#include <tvterm/vtermframe.h>
#include <tvterm/vtermview.h>

TVTermFrame::TVTermFrame(const TRect &bounds) :
    TFrame(bounds),
    term(nullptr)
{
}

void TVTermFrame::draw()
{
    TFrame::draw();
    if (term)
        drawSize();
}

void TVTermFrame::drawSize()
{
    if (state & sfDragging)
    {
        TRect r(4, size.y - 1, min(size.x - 4, 14), size.y);
        if (r.a.x < r.b.x && r.a.y < r.b.y)
        {
            TDrawBuffer b;
            uchar color = mapColor(5);
            ushort width;
            {
                char str[256];
                sprintf(str, " %dx%d ", term->size.x, term->size.y);
                width = b.moveStr(0, str, color, r.b.x - r.a.x);
            }
            writeLine(r.a.x, r.a.y, width, r.b.y - r.a.y, b);
        }
    }
}
