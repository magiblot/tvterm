#ifndef TVTERM_CMDS_H
#define TVTERM_CMDS_H

#include <tvision/tv.h>

enum : ushort
{
    cmGrabInput = 100,
    cmReleaseInput,
    cmTileCols,
    cmTileRows,
    // Commands that cannot be deactivated.
    cmNewTerm = 1000,
    cmCheckTerminalUpdates,
    cmTerminalUpdated,
    cmGetOpenTerms,
    cmQuickMenu,
};

enum : ushort
{
    hcInputGrabbed = 1000,
    hcQuickMenu,
};

#endif // TVTERM_CMDS_H
