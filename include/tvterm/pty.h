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

    int master_fd;
    pid_t child_pid;

public:

    template <class Func>
    PTY(TPoint, Func &&);
    ~PTY();

    int getMaster() const;
    bool isValid() const;
    bool getSize(TPoint &) const;
    bool setSize(TPoint) const;
    bool setBlocking(bool) const;
    ssize_t read(TSpan<char>) const;
    ssize_t write(TSpan<const char>) const;

};

template <class Func>
inline PTY::PTY(TPoint size, Func &&func)
{
    struct termios termios; initTermios(&termios);
    struct winsize winsize; initWinsize(&winsize, size);

    child_pid = forkpty(&master_fd, nullptr, &termios, &winsize);
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
        // Don't clean up resources, because we didn't initiate them
        // (e.g. terminal state).
        _exit(1);
    }
}

inline int PTY::getMaster() const
{
    return master_fd;
}

inline bool PTY::isValid() const
{
    return child_pid != -1;
}

#endif // TVTERM_PTY_H
