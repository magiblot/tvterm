#ifndef TVTERM_PTY_H
#define TVTERM_PTY_H

#define Uses_TPoint
#include <tvision/tv.h>

#include <tvterm/debug.h>
#include <tvterm/util.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <cstdlib>

#if defined(__FreeBSD__)
#include <sys/ioctl.h>
#include <libutil.h>
#include <termios.h>
#elif defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
#include <sys/ioctl.h>
#include <termios.h>
#include <util.h>
#else
#include <pty.h>
#endif

class PTY
{

    static void initTermios(struct termios *);
    static void initWinsize(struct winsize *, TPoint);

    int master_fd {-1};
    pid_t child_pid {-1};

    PTY(int master_fd, pid_t child_pid) :
        master_fd(master_fd),
        child_pid(child_pid)
    {
    }

public:

    PTY() = default;
    PTY& operator=(const PTY &) = delete;
    PTY& operator=(PTY &&) = default;

    template <class Func>
    static PTY create(TPoint, Func &&);
    void destroy();

    int getMaster() const;
    bool getSize(TPoint &) const;
    bool setSize(TPoint) const;
    bool setBlocking(bool) const;
    ssize_t read(TSpan<char>) const;
    ssize_t write(TSpan<const char>) const;

};

template <class Func>
inline PTY PTY::create(TPoint size, Func &&func)
{
    struct termios termios; initTermios(&termios);
    struct winsize winsize; initWinsize(&winsize, size);

    int master_fd;
    pid_t child_pid = forkpty(&master_fd, nullptr, &termios, &winsize);
    if (child_pid == 0)
    {
        // Restore the ISIG signals back to defaults
        signal(SIGINT,  SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGSTOP, SIG_DFL);
        signal(SIGCONT, SIG_DFL);

        func();

        char *shell = getenv("SHELL");
        char *args[] = {shell, nullptr};
        execvp(shell, args);
        dout << "Child: cannot exec " << shell << ": " << strerror(errno)
             << endl << flush;
        _exit(1);
    }
    return {master_fd, child_pid};
}

inline int PTY::getMaster() const
{
    return master_fd;
}

#endif // TVTERM_PTY_H
