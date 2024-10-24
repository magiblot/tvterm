#ifndef TVTERM_APPUTIL_H
#define TVTERM_APPUTIL_H

#define Uses_TProgram
#define Uses_TDeskTop
#define Uses_TMenuPopup
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

class CenteredMenuPopup : public TMenuPopup
{
public:

    CenteredMenuPopup(TMenu *aMenu) noexcept :
        TMenuPopup(TRect(0, 0, 0, 0), aMenu)
    {
        options |= ofCentered;
    }

    void calcBounds(TRect &bounds, TPoint delta) override
    {
        bounds.a.x = (owner->size.x - size.x)/2;
        bounds.a.y = (owner->size.y - size.y)/2;
        bounds.b.x = bounds.a.x + size.x;
        bounds.b.y = bounds.a.y + size.y;
    }
};

#endif // TVTERM_APPUTIL_H
