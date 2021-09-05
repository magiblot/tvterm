#include <tvterm/desk.h>

TVTermDesk::TVTermDesk(const TRect &bounds) :
    TDeskInit(&TDeskTop::initBackground),
    TDeskTop(bounds)
{
}

void TVTermDesk::tileWithOrientation(bool columnsFirst, const TRect &bounds)
{
    bool bak = tileColumnsFirst;
    tileColumnsFirst = columnsFirst;
    tile(bounds);
    tileColumnsFirst = bak;
}

void TVTermDesk::tileVertical(const TRect &bounds)
{
    tileWithOrientation(true, bounds);
}

void TVTermDesk::tileHorizontal(const TRect &bounds)
{
    tileWithOrientation(false, bounds);
}

