#ifndef TVTERM_UTIL_H
#define TVTERM_UTIL_H

#define Uses_TDialog
#define Uses_TProgram
#define Uses_TDeskTop
#include <tvision/tv.h>

ushort execDialog(TDialog *d)
{
    TView *p = TProgram::application->validView(d);
    if (p)
    {
        ushort result = TProgram::deskTop->execView(p);
        TObject::destroy(p);
        return result;
    }
    return cmCancel;
}

int fd_set_flags(int fd, int flags);

#endif // TVTERM_UTIL_H
