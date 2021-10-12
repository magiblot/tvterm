#ifndef TVTERM_DESK_H
#define TVTERM_DESK_H

#define Uses_TDeskTop
#include <tvision/tv.h>

class TVTermDesk : public TDeskTop
{

    void tileWithOrientation(bool columnsFirst, const TRect &bounds);

public:

    TVTermDesk(const TRect &bounds);

    void tileVertical(const TRect &bounds);
    void tileHorizontal(const TRect &bounds);

};

#endif // TVTERM_DESK_H
