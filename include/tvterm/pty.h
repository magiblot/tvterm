#ifndef TVTERM_PTY_H
#define TVTERM_PTY_H

#include <stddef.h>

#include <sys/types.h>

template <class T>
class TSpan;
class TPoint;

namespace tvterm
{

struct PtyDescriptor
{
    int masterFd;
    pid_t clientPid;

    bool valid() const
    {
        return masterFd != -1;
    }
};

struct EnvironmentVar
{
    const char *name;
    const char *value;
};

PtyDescriptor createPty( TPoint size, TSpan<const EnvironmentVar> environment,
                         void (&onError)(const char *reason) ) noexcept;

class PtyMaster
{
    int masterFd;
    pid_t clientPid;

public:

    PtyMaster(PtyDescriptor ptyDescriptor) noexcept;

    bool readFromClient(TSpan<char> data, size_t &bytesRead) noexcept;
    bool writeToClient(TSpan<const char> data) noexcept;
    void resizeClient(TPoint size) noexcept;
    void disconnect() noexcept;
};

inline PtyMaster::PtyMaster(PtyDescriptor ptyDescriptor) noexcept :
    masterFd(ptyDescriptor.masterFd),
    clientPid(ptyDescriptor.clientPid)
{
}

} // namespace tvterm

#endif // TVTERM_PTY_H
