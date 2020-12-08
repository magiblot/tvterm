#ifndef TVTERM_CMDS_H
#define TVTERM_CMDS_H

#include <tvision/tv.h>

enum : ushort {
    // Commands that cannot be deactivated.
    cmNewTerm           = 1000,
    cmVTermReadable,
};

#endif // TVTERM_CMDS_H
