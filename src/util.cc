#include <tvterm/util.h>
#include <fcntl.h>

int fd_set_flags(int fd, int flags)
{
    int oldFlags = fcntl(fd, F_GETFL);
    if (oldFlags != -1)
        return fcntl(fd, F_SETFL, oldFlags | flags);
    return -1;
}

int fd_unset_flags(int fd, int flags)
{
    int oldFlags = fcntl(fd, F_GETFL);
    if (oldFlags != -1)
        return fcntl(fd, F_SETFL, oldFlags & ~flags);
    return -1;
}
