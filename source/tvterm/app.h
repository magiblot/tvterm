#ifndef TVTERM_APP_H
#define TVTERM_APP_H

#define Uses_TApplication
#define Uses_TCommandSet
#include <tvision/tv.h>

#include <tvterm/threadpool.h>

class TVTermDesk;

struct TVTermApp : public TApplication
{
    static TVTermApp *app;
    static TCommandSet tileCmds;

    tvterm::ThreadPool threadPool;

    TVTermApp();
    static TMenuBar* initMenuBar(TRect r);
    static TStatusLine* initStatusLine(TRect r);
    static TDeskTop* initDeskTop(TRect r);

    TVTermDesk* getDeskTop();

    void handleEvent(TEvent &event) override;
    Boolean valid(ushort command) override;
    void idle() override;

private:
    size_t getOpenTermCount();
    TRect newWindowCoords();

    // Command handlers
public:
    void newTerm();
    void changeDir();
};

inline TVTermDesk* TVTermApp::getDeskTop()
{
    return (TVTermDesk*) deskTop;
}

#endif // TVTERM_APP_H
