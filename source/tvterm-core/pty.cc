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

        setbuf(stderr, nullptr); // Ensure 'stderr' is unbuffered.
        fprintf(
            stderr,
            "\x1B[1;31m" // Color attributes: bold and red.
            "Error: Failed to execute the program specified by the environment variable SHELL ('%s'): %s"
            "\x1B[0m", // Reset color attributes.
            (shell ? shell : ""),
            strerror(errno)
        );

        // Exit the subprocess without cleaning up resources.
        _Exit(EXIT_FAILURE);
    }
    else if (child_pid == -1)
    {
        char *str = fmtStr("forkpty failed: %s", strerror(errno));
        onError(str);
        delete[] str;
        return {-1};
    }
    return {master_fd};
}

bool PtyMaster::readFromClient(TSpan<char> data, size_t &bytesRead) noexcept
{
    bytesRead = 0;
    if (data.size() > 1)
    {
        ssize_t r = read(fd, &data[0], 1);
        if (r < 0)
            return false;
        else if (r > 0)
        {
            bytesRead += r;
            int availableBytes = 0;
            if ( ioctl(fd, FIONREAD, &availableBytes) != -1 &&
                 availableBytes > 0 )
            {
                r = read(fd, &data[1], min(availableBytes, data.size() - 1));
                if (r < 0)
                    return false;
                bytesRead += r;
            }
        }
    }
    return true;
}

bool PtyMaster::writeToClient(TSpan<const char> data) noexcept
{
    size_t written = 0;
    while (written < data.size())
    {
        ssize_t r = write(fd, &data[written], data.size() - written);
        if (r < 0)
            return false;
        written += r;
    }
    return true;
}

void PtyMaster::resizeClient(TPoint size) noexcept
{
    struct winsize w = {};
    w.ws_row = size.y;
    w.ws_col = size.x;
    int rr = ioctl(fd, TIOCSWINSZ, &w);
    (void) rr;
}

void PtyMaster::disconnect() noexcept
{
    close(fd);
}

static struct winsize createWinsize(TPoint size) noexcept
{
    struct winsize w = {};
    w.ws_row = size.y;
    w.ws_col = size.x;
    return w;
}

static struct termios createTermios() noexcept
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
