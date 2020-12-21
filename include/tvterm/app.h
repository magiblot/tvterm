#ifndef TVTERM_APP_H
#define TVTERM_APP_H

#define Uses_TApplication
#define Uses_TCommandSet
#include <tvision/tv.h>

class TVTermDesk;

struct TVTermApp : public TApplication
{
    static TVTermApp *app;
    static TCommandSet tileCmds;

    TVTermApp();
    static TMenuBar* initMenuBar(TRect r);
    static TStatusLine* initStatusLine(TRect r);
    static TDeskTop* initDeskTop(TRect r);

    TVTermDesk* getDeskTop();

    void getEvent(TEvent &event) override;
    void handleEvent(TEvent &event) override;
    void idle() override;

    // Command handlers

    void newTerm();
    void changeDir();

};

inline TVTermDesk* TVTermApp::getDeskTop()
{
    return (TVTermDesk*) deskTop;
}

#endif // TVTERM_APP_H
