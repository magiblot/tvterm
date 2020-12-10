#ifndef TVTERM_VTERMWND_H
#define TVTERM_VTERMWND_H

#define Uses_TWindow
#define Uses_TCommandSet
#include <tvision/tv.h>

#include <string_view>

struct TVTermWindow : public TWindow
{

    TVTermWindow(const TRect &bounds);

    std::string_view termTitle;

    static TCommandSet focusedCmds;

    void setTitle(std::string_view);

    void handleEvent(TEvent &ev) override;
    void setState(ushort aState, Boolean enable) override;
    ushort execute() override;

};

#endif // TVTERM_VTERMWND_H
