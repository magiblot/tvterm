#ifndef TVTERM_VTERMWND_H
#define TVTERM_VTERMWND_H

#define Uses_TWindow
#include <tvision/tv.h>

struct VTermWindow : public TWindow
{

    VTermWindow(const TRect &bounds);

};

#endif // TVTERM_VTERMWND_H
