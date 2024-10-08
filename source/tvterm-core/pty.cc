#include <tvterm/debug.h>
#include <tvterm/pty.h>

#define Uses_TPoint
#include <tvision/tv.h>

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

#if __has_include(<pty.h>)
#   include <pty.h>
#elif __has_include(<libutil.h>)
#   include <libutil.h>
#elif __has_include(<util.h>)
#   include <util.h>
#endif

namespace tvterm
{

static struct termios createTermios() noexcept;
static struct winsize createWinsize(TPoint size) noexcept;

PtyDescriptor createPty( TPoint size, TSpan<const EnvironmentVar> customEnvironment,
                         void (&onError)(const char *) ) noexcept
{
    auto termios = createTermios();
    auto winsize = createWinsize(size);
    int master_fd;
    pid_t child_pid = forkpty(&master_fd, nullptr, &termios, &winsize);
    if (child_pid == 0)
    {
        // Use the default ISIG signal handlers.
        signal(SIGINT,  SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGSTOP, SIG_DFL);
        signal(SIGCONT, SIG_DFL);

        for (const auto &envVar : customEnvironment)
            setenv(envVar.name, envVar.value, 1);

        char *shell = getenv("SHELL");
        char *args[] = {shell, nullptr};
        execvp(shell, args);
        dout << "Child: cannot exec " << (shell ? shell : "(nil)") << ": " << strerror(errno)
             << std::endl << std::flush;
        // Don't clean up resources, because we didn't initiate them
        // (e.g. terminal state).
        _exit(1);
    }
    else if (child_pid == -1)
    {
        char str[256];
        snprintf(str, sizeof(str), "forkpty failed: %s", strerror(errno));
        onError(str);
        return {-1, -1};
    }
    return {master_fd, child_pid};
}

PtyProcess::~PtyProcess()
{
    close(master_fd);
    kill(child_pid, SIGINT);
    kill(child_pid, SIGTERM);
}

TPoint PtyProcess::getSize() const noexcept
{
    struct winsize w;
    if (ioctl(master_fd, TIOCGWINSZ, &w) != -1)
        return {w.ws_col, w.ws_row};
    return {};
}

void PtyProcess::setSize(TPoint size) const noexcept
{
    if (size != getSize())
    {
        struct winsize w = {};
        w.ws_row = size.y;
        w.ws_col = size.x;
        int rr = ioctl(master_fd, TIOCSWINSZ, &w);
        (void) rr;
    }
}

ssize_t PtyProcess::read(TSpan<char> buf) const noexcept
{
    return ::read(master_fd, buf.data(), buf.size());
}

ssize_t PtyProcess::write(TSpan<const char> buf) const noexcept
{
    return ::write(master_fd, buf.data(), buf.size());
}

struct winsize createWinsize(TPoint size) noexcept
{
    struct winsize w = {};
    w.ws_row = size.y;
    w.ws_col = size.x;
    return w;
}

struct termios createTermios() noexcept
{
    // Initialization like in pangoterm.
    struct termios t = {};

    t.c_iflag = ICRNL | IXON;
    t.c_oflag = OPOST | ONLCR
#ifdef TAB0
        | TAB0
#endif
        ;
    t.c_cflag = CS8 | CREAD;
    t.c_lflag = ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK;

#ifdef IUTF8
    t.c_iflag |= IUTF8;
#endif
#ifdef NL0
    t.c_oflag |= NL0;
#endif
#ifdef CR0
    t.c_oflag |= CR0;
#endif
#ifdef BS0
    t.c_oflag |= BS0;
#endif
#ifdef VT0
    t.c_oflag |= VT0;
#endif
#ifdef FF0
    t.c_oflag |= FF0;
#endif
#ifdef ECHOCTL
    t.c_lflag |= ECHOCTL;
#endif
#ifdef ECHOKE
    t.c_lflag |= ECHOKE;
#endif

    t.c_cc[VINTR]    = 0x1f & 'C';
    t.c_cc[VQUIT]    = 0x1f & '\\';
    t.c_cc[VERASE]   = 0x7f;
    t.c_cc[VKILL]    = 0x1f & 'U';
    t.c_cc[VEOF]     = 0x1f & 'D';
    t.c_cc[VEOL]     = _POSIX_VDISABLE;
    t.c_cc[VEOL2]    = _POSIX_VDISABLE;
    t.c_cc[VSTART]   = 0x1f & 'Q';
    t.c_cc[VSTOP]    = 0x1f & 'S';
    t.c_cc[VSUSP]    = 0x1f & 'Z';
    t.c_cc[VREPRINT] = 0x1f & 'R';
    t.c_cc[VWERASE]  = 0x1f & 'W';
    t.c_cc[VLNEXT]   = 0x1f & 'V';
    t.c_cc[VMIN]     = 1;
    t.c_cc[VTIME]    = 0;

    cfsetispeed(&t, B38400);
    cfsetospeed(&t, B38400);

    return t;
}

} // namespace tvterm
