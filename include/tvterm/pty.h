#ifndef TVTERM_PTY_H
#define TVTERM_PTY_H

#include <sys/types.h>

#if __has_include(<pty.h>)
#   include <pty.h>
#elif __has_include(<libutil.h>)
#   include <libutil.h>
#elif __has_include(<util.h>)
#   include <util.h>
#endif

template <class T>
class TSpan;
class TPoint;

namespace tvterm
{

struct PtyUtil
{
    decltype(::forkpty) *forkpty;
};

class DynLib;

class DynPtyUtil : public PtyUtil
{
    DynLib *lib;

public:

    DynPtyUtil() noexcept;
    ~DynPtyUtil();
};

struct PtyDescriptor
{
    int master_fd;
    pid_t child_pid;

    bool valid() const
    {
        return master_fd != -1;
    }
};

PtyDescriptor createPty( TPoint size, void (&doAsChild)(),
                         void (&onError)(const char *reason),
                         const PtyUtil &ptyUtil ) noexcept;

class PtyProcess
{
    int master_fd;
    pid_t child_pid;

public:

    PtyProcess(PtyDescriptor ptyDescriptor) noexcept;
    ~PtyProcess();

    int getMaster() const noexcept;
    TPoint getSize() const noexcept;
    void setSize(TPoint) const noexcept;
    ssize_t read(TSpan<char>) const noexcept;
    ssize_t write(TSpan<const char>) const noexcept;

};

inline PtyProcess::PtyProcess(PtyDescriptor ptyDescriptor) noexcept :
    master_fd(ptyDescriptor.master_fd),
    child_pid(ptyDescriptor.child_pid)
{
}

inline int PtyProcess::getMaster() const noexcept
{
    return master_fd;
}

} // namespace tvterm

#endif // TVTERM_PTY_H
