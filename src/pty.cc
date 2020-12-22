#include <tvterm/pty.h>
#include <fcntl.h>

PTY::~PTY()
{
    close(master_fd);
    kill(child_pid, SIGINT);
    kill(child_pid, SIGTERM);
}

bool PTY::getSize(TPoint &size) const
{
    struct winsize w;
    if (ioctl(master_fd, TIOCGWINSZ, &w) != -1)
    {
        size = {w.ws_col, w.ws_row};
        return true;
    }
    size = {};
    return false;
}

bool PTY::setSize(TPoint size) const
{
    struct winsize w = {};
    w.ws_row = size.y;
    w.ws_col = size.x;
    return ioctl(master_fd, TIOCSWINSZ, &w) != -1;
}

bool PTY::setBlocking(bool enable) const
{
    if (enable)
        return fd_unset_flags(master_fd, O_NONBLOCK);
    else
        return fd_set_flags(master_fd, O_NONBLOCK);
}

ssize_t PTY::read(TSpan<char> buf) const
{
    return ::read(master_fd, buf.data(), buf.size());
}

ssize_t PTY::write(TSpan<const char> buf) const
{
    return ::write(master_fd, buf.data(), buf.size());
}

void PTY::initWinsize(struct winsize *winsize, TPoint size)
{
    struct winsize w = {};

    w.ws_row = size.y;
    w.ws_col = size.x;

    *winsize = w;
}

void PTY::initTermios(struct termios *termios)
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

    cfsetispeed(&t, B38400);
    cfsetospeed(&t, B38400);

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

    *termios = t;
}
