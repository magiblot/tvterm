#ifndef TVTERM_VTERMWND_H
#define TVTERM_VTERMWND_H

#define Uses_TWindow
#include <tvision/tv.h>

#include <string_view>

struct TVTermWindow : public TWindow
{

    TVTermWindow(const TRect &bounds);

    std::string_view termTitle;

    void setTitle(std::string_view);

    void handleEvent(TEvent &ev) override;
    ushort execute() override;

};

#endif // TVTERM_VTERMWND_H
