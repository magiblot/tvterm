#ifndef TVTERM_APPUTIL_H
#define TVTERM_APPUTIL_H

#define Uses_TProgram
#define Uses_TDeskTop
#include <tvision/tv.h>

inline ushort execDialog(TDialog *d)
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

#endif // TVTERM_APPUTIL_H
