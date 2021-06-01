#ifndef TVTERM_VTERMWND_H
#define TVTERM_VTERMWND_H

#define Uses_TWindow
#define Uses_TCommandSet
#include <tvision/tv.h>

#include <tvterm/io.h>

#include <string_view>

struct TVTermView;

struct TVTermWindow : public TWindow
{

    static TFrame *initFrame(TRect);

    TVTermWindow(const TRect &bounds, asio::io_context &io);

    std::string_view termTitle;
    TVTermView *view;

    static TCommandSet focusedCmds;

    void shutDown() override;
    void setTitle(std::string_view);
    bool ptyClosed() const;

    void handleEvent(TEvent &ev) override;
    void setState(ushort aState, Boolean enable) override;
    ushort execute() override;

};

#endif // TVTERM_VTERMWND_H
