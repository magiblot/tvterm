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
};

#endif // TVTERM_APP_H
