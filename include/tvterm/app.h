#ifndef TVTERM_APP_H
#define TVTERM_APP_H

#define Uses_TApplication
#include <tvision/tv.h>

const ushort
// Commands that cannot be deactivated.
    cmNewTerm       = 1000;

struct TVTermApp : public TApplication
{
    static TVTermApp *app;

    TVTermApp();
    static TMenuBar* initMenuBar(TRect r);
    static TStatusLine* initStatusLine(TRect r);

    void handleEvent(TEvent &event) override;

    // Command handlers

    void newTerm();
    void changeDir();
    void shell();
    void tile();
    void cascade();

};

#endif // TVTERM_APP_H
