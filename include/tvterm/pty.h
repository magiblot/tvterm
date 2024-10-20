#ifndef TVTERM_PTY_H
#define TVTERM_PTY_H

#include <stddef.h>

template <class T>
class TSpan;
class TPoint;

namespace tvterm
{

struct PtyDescriptor
{
    int fd;

    bool valid() const
    {
        return fd != -1;
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
    int fd;

public:

    PtyMaster(PtyDescriptor ptyDescriptor) noexcept;

    bool readFromClient(TSpan<char> data, size_t &bytesRead) noexcept;
    bool writeToClient(TSpan<const char> data) noexcept;
    void resizeClient(TPoint size) noexcept;
    void disconnect() noexcept;
};

inline PtyMaster::PtyMaster(PtyDescriptor ptyDescriptor) noexcept :
    fd(ptyDescriptor.fd)
{
}

} // namespace tvterm

#endif // TVTERM_PTY_H
