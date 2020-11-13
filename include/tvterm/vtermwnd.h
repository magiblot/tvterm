#ifndef TVTERM_VTERMWND_H
#define TVTERM_VTERMWND_H

#define Uses_TWindow
#include <tvision/tv.h>

struct TVTermWindow : public TWindow
{

    TVTermWindow(const TRect &bounds);

};

#endif // TVTERM_VTERMWND_H
