#ifndef TVTERM_PTY_H
#define TVTERM_PTY_H

#include <sys/types.h>

template <class T>
class TSpan;
class TPoint;

namespace tvterm
{

struct PtyDescriptor
{
    int master_fd;
    pid_t child_pid;

    bool valid() const
    {
        return master_fd != -1;
    }
};

struct EnvironmentVar
{
    const char *name;
    const char *value;
};

PtyDescriptor createPty( TPoint size, TSpan<const EnvironmentVar> environment,
                         void (&onError)(const char *reason) ) noexcept;

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
