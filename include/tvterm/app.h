#ifndef TVTERM_APP_H
#define TVTERM_APP_H

#define Uses_TApplication
#define Uses_TCommandSet
#include <tvision/tv.h>

struct TVTermApp : public TApplication
{
    static TVTermApp *app;
    static TCommandSet tileCmds;

    TVTermApp();
    static TMenuBar* initMenuBar(TRect r);
    static TStatusLine* initStatusLine(TRect r);

    void getEvent(TEvent &event) override;
    void handleEvent(TEvent &event) override;
    void idle() override;

    // Command handlers

    void newTerm();
    void changeDir();

};

#endif // TVTERM_APP_H
