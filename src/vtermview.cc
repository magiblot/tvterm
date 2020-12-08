#define Uses_TText
#include <tvision/tv.h>

#include <tvterm/vtermview.h>
#include <tvterm/vtermwnd.h>
#include <tvterm/ptylisten.h>
#include <tvterm/vterm.h>
#include <tvterm/pty.h>
#include <tvterm/util.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

TVTermView::TVTermView(const TRect &bounds, TVTermWindow &window) :
    TView(bounds),
    window(window),
    vterm(*this)
{
    growMode = gfGrowHiX | gfGrowHiY;
    options |= ofSelectable | ofFirstClick;
    showCursor();
}

inline TScreenCell& TVTermView::at(int y, int x)
{
    // Temporary solution: draw directly on the owner's buffer.
    TRect r = getBounds();
    return owner->buffer[owner->size.x * (r.a.y + y) + (r.a.x + x)];
}

const VTermScreenCallbacks TVTermAdapter::callbacks = {
    static_wrap<&TVTermAdapter::damage>,
    static_wrap<&TVTermAdapter::moverect>,
    static_wrap<&TVTermAdapter::movecursor>,
    static_wrap<&TVTermAdapter::settermprop>,
    static_wrap<&TVTermAdapter::bell>,
    static_wrap<&TVTermAdapter::resize>,
    static_wrap<&TVTermAdapter::sb_pushline>,
    static_wrap<&TVTermAdapter::sb_popline>,
};

TVTermAdapter::TVTermAdapter(TVTermView &view) :
    view(view),
    listener(nullptr),
    pending(false)
{
    vt = vterm_new(view.size.y, view.size.x);
    vterm_set_utf8(vt, 1);

    state = vterm_obtain_state(vt);
    vterm_state_reset(state, true);

    vts = vterm_obtain_screen(vt);
    vterm_screen_set_callbacks(vts, &callbacks, this);

    struct termios termios;
    struct winsize winsize;
    initTermios(termios, winsize);

    child_pid = forkpty(&master_fd, nullptr, &termios, &winsize);
    if (child_pid == 0)
    {
        // Restore the ISIG signals back to defaults
        signal(SIGINT,  SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGSTOP, SIG_DFL);
        signal(SIGCONT, SIG_DFL);
        char *shell = getenv("SHELL");
        char *args[] = { shell, nullptr };
        execvp(shell, args);
        cerr << "Child: cannot exec " << shell << ": " << strerror(errno) << endl;
        cerr << flush;
        _exit(1);
    }
    else
    {
        fd_set_flags(master_fd, O_NONBLOCK);
        listener = new PTYListener(*this);
    }
}

TVTermAdapter::~TVTermAdapter()
{
    delete listener;
    cerr << "close(" << master_fd << ") = " << ::close(master_fd) << endl;
    cerr << "waitpid(" << child_pid <<") = " << flush
         << waitpid(child_pid, nullptr, 0) << endl;
    vterm_free(vt);
}

void TVTermAdapter::initTermios( struct termios &termios,
                                 struct winsize &winsize ) const
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

    termios = t;

    struct winsize w = {};
    w.ws_row = view.size.y;
    w.ws_col = view.size.x;
    winsize = w;
}

void TVTermAdapter::read()
{
    if (pending)
    {
        pending = false;
        char buf[4096];
        ssize_t size;
        while ((size = ::read(master_fd, buf, sizeof(buf))) > 0)
        {
            cerr << master_fd << ": read " << size << " bytes." << endl;
            vterm_input_write(vt, buf, size);
        }
        vterm_screen_flush_damage(vts);
    }
}

// These functions are inline because they may only be invoked from the
// 'TVTermAdapter::callbacks' table, which contains references to functions
// generated at compile time.

inline int TVTermAdapter::damage(VTermRect rect)
{
    TRect r(rect.start_col, rect.start_row, rect.end_col, rect.end_row);
    r.intersect(view.getExtent());
    for (int y = r.a.y; y < r.b.y; ++y)
    {
        TSpan<TScreenCell> cells(&view.at(y, r.a.x), r.b.x - r.a.x);
        size_t ci = 0;
        for (int x = r.a.x; x < r.b.x;)
        {
            VTermScreenCell cell;
            vterm_screen_get_cell(vts, {y, x}, &cell);
            {
                char text[4*VTERM_MAX_CHARS_PER_CELL];
                VTermRect pos = {y, y + 1, x, x + 1};
                size_t textLength = vterm_screen_get_text(vts, text, sizeof(text), pos);
                size_t ti = 0;
                while (TText::eat(cells, ci, {text, textLength}, ti))
                    ;
            }
            x += cell.width;
        }
    }
    view.window.drawView();
    return 0;
}

inline int TVTermAdapter::moverect(VTermRect dest, VTermRect src)
{
    return 0;
}

inline int TVTermAdapter::movecursor(VTermPos pos, VTermPos oldpos, int visible)
{
    if (visible)
        view.showCursor();
    else
        view.hideCursor();
    view.setCursor(pos.col, pos.row);
    return 0;
}

inline int TVTermAdapter::settermprop(VTermProp prop, VTermValue *val)
{
    return 0;
}

inline int TVTermAdapter::bell()
{
    return 0;
}

inline int TVTermAdapter::resize(int rows, int cols)
{
    return 0;
}

inline int TVTermAdapter::sb_pushline(int cols, const VTermScreenCell *cells)
{
    return 0;
}

inline int TVTermAdapter::sb_popline(int cols, VTermScreenCell *cells)
{
    return 0;
}
