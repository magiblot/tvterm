#ifndef TVTERM_VTERMWND_H
#define TVTERM_VTERMWND_H

#define Uses_TWindow
#include <tvision/tv.h>

#include <string_view>

struct TVTermWindow : public TWindow
{

    TVTermWindow(const TRect &bounds);

    void setTitle(std::string_view);

};

#endif // TVTERM_VTERMWND_H
