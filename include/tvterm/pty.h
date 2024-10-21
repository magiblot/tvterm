#ifndef TVTERM_PTY_H
#define TVTERM_PTY_H

#include <stddef.h>

#if !defined(_WIN32)
#   include <sys/types.h>
#else
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <tvision/compat/windows/windows.h>
#   ifdef small
#       undef small
#   endif
#endif

template <class T>
class TSpan;
class TPoint;

namespace tvterm
{

struct PtyDescriptor
{
#if !defined(_WIN32)
    int masterFd;
    pid_t clientPid;
#else
    HANDLE hMasterRead;
    HANDLE hMasterWrite;
    HPCON hPseudoConsole;
    HANDLE hClientProcess;
#endif
};

struct EnvironmentVar
{
    const char *name;
    const char *value;
};

bool createPty( PtyDescriptor &ptyDescriptor,
                TPoint size,
                TSpan<const EnvironmentVar> environment,
                void (&onError)(const char *reason) ) noexcept;

class PtyMaster
{
    PtyDescriptor d;

public:

    PtyMaster(PtyDescriptor ptyDescriptor) noexcept;

    bool readFromClient(TSpan<char> data, size_t &bytesRead) noexcept;
    bool writeToClient(TSpan<const char> data) noexcept;
    void resizeClient(TPoint size) noexcept;
    void disconnect() noexcept;
};

inline PtyMaster::PtyMaster(PtyDescriptor ptyDescriptor) noexcept :
    d(ptyDescriptor)
{
}

} // namespace tvterm

#endif // TVTERM_PTY_H
