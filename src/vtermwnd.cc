#define Uses_TFrame
#include <tvision/tv.h>

#include <tvterm/vtermwnd.h>
#include <tvterm/vtermview.h>

TVTermWindow::TVTermWindow(const TRect &bounds) :
    TWindowInit(&TVTermWindow::initFrame),
    TWindow(bounds, nullptr, wnNoNumber)
{
    options |= ofTileable;
    setState(sfShadow, False);
    {
        TRect r = getExtent().grow(-1, -1);
        TView *vt = new TVTermView(r, *this);
        insert(vt);
    }
}

void TVTermWindow::setTitle(std::string_view text)
{
    delete[] title;
    // For some unknown reason the last character is eaten up by TFrame...
    // We'll find out why later. Let's work around it for the time being.
    char *str = new char[text.size() + 2];
    memcpy(str, text.data(), text.size());
    str[text.size()] = ' ';
    str[text.size() + 1] = '\0';
    title = str;
    frame->drawView();
}
