#ifndef TVTERM_VTERM_H
#define TVTERM_VTERM_H

#include <vterm.h>

inline void vterm_set_size_safe(VTerm *vt, int rows, int cols)
{
    vterm_set_size(vt, rows > 0 ? rows : 1,
                       cols > 0 ? cols : 1);
}

#endif // TVTERM_VTERM_H
