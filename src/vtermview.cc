#define Uses_TText
#define Uses_TKeys
#include <tvision/tv.h>

#include <tvterm/vtermview.h>
#include <tvterm/vtermwnd.h>
#include <tvterm/ptylisten.h>
#include <tvterm/vterm.h>
#include <tvterm/pty.h>
#include <tvterm/util.h>
#include <unordered_map>
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
    eventMask |= evMouseUp | evMouseMove | evMouseAuto;
    showCursor();
}

inline TScreenCell& TVTermView::at(int y, int x)
{
    // Temporary solution: draw directly on the owner's buffer.
    TRect r = getBounds();
    return owner->buffer[owner->size.x * (r.a.y + y) + (r.a.x + x)];
}

void TVTermView::changeBounds(const TRect& bounds)
{
    TView::changeBounds(bounds);
    vterm.setSize(size.y, size.x);
}

void TVTermView::handleEvent(TEvent &ev)
{
    TView::handleEvent(ev);
    vterm.handleEvent(ev);
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

static const std::unordered_map<ushort, VTermKey> tv_vterm_keys = {
    { kbEnter,          VTERM_KEY_ENTER     },
    { kbCtrlEnter,      VTERM_KEY_ENTER     },
    { kbTab,            VTERM_KEY_TAB       },
    { kbShiftTab,       VTERM_KEY_TAB       },
    { kbBack,           VTERM_KEY_BACKSPACE },
    { kbAltBack,        VTERM_KEY_BACKSPACE },
    { kbCtrlBack,       VTERM_KEY_BACKSPACE },
    { kbEsc,            VTERM_KEY_ESCAPE    },
    { kbUp,             VTERM_KEY_UP        },
    { kbAltUp,          VTERM_KEY_UP        },
    { kbCtrlUp,         VTERM_KEY_UP        },
    { kbDown,           VTERM_KEY_DOWN      },
    { kbAltDown,        VTERM_KEY_DOWN      },
    { kbCtrlDown,       VTERM_KEY_DOWN      },
    { kbLeft,           VTERM_KEY_LEFT      },
    { kbAltLeft,        VTERM_KEY_LEFT      },
    { kbCtrlLeft,       VTERM_KEY_LEFT      },
    { kbRight,          VTERM_KEY_RIGHT     },
    { kbAltRight,       VTERM_KEY_RIGHT     },
    { kbCtrlRight,      VTERM_KEY_RIGHT     },
    { kbIns,            VTERM_KEY_INS       },
    { kbAltIns,         VTERM_KEY_INS       },
    { kbCtrlIns,        VTERM_KEY_INS       },
    { kbShiftIns,       VTERM_KEY_INS       },
    { kbDel,            VTERM_KEY_DEL       },
    { kbAltDel,         VTERM_KEY_DEL       },
    { kbCtrlDel,        VTERM_KEY_DEL       },
    { kbShiftDel,       VTERM_KEY_DEL       },
    { kbHome,           VTERM_KEY_HOME      },
    { kbAltHome,        VTERM_KEY_HOME      },
    { kbCtrlHome,       VTERM_KEY_HOME      },
    { kbEnd,            VTERM_KEY_END       },
    { kbAltEnd,         VTERM_KEY_END       },
    { kbCtrlEnd,        VTERM_KEY_END       },
    { kbPgUp,           VTERM_KEY_PAGEUP    },
    { kbAltPgUp,        VTERM_KEY_PAGEUP    },
    { kbCtrlPgUp,       VTERM_KEY_PAGEUP    },
    { kbPgDn,           VTERM_KEY_PAGEDOWN  },
    { kbAltPgDn,        VTERM_KEY_PAGEDOWN  },
    { kbCtrlPgDn,       VTERM_KEY_PAGEDOWN  },
};

static constexpr struct { ushort tvmod; VTermModifier vtmod; } tv_vterm_modifiers[] = {
    { kbShift,      VTERM_MOD_SHIFT },
    { kbAltShift,   VTERM_MOD_ALT   },
    { kbCtrlShift,  VTERM_MOD_CTRL  },
};

static VTermKey tv_vterm_conv_key(ushort keyCode)
{
    auto it = tv_vterm_keys.find(keyCode);
    if (it != tv_vterm_keys.end())
        return it->second;
    return VTERM_KEY_NONE;
}

static VTermModifier tv_vterm_conv_mod(ulong controlKeyState)
{
    VTermModifier mod = VTERM_MOD_NONE;
    for (const auto [tvmod, vtmod] : tv_vterm_modifiers)
        if (controlKeyState & tvmod)
            mod = VTermModifier(mod | vtmod);
    return mod;
}

TVTermAdapter::TVTermAdapter(TVTermView &view) :
    view(view),
    listener(nullptr),
    pending(false),
    resizing(false),
    mouseEnabled(false)
{
    vt = vterm_new(view.size.y, view.size.x);
    vterm_set_utf8(vt, 1);

    state = vterm_obtain_state(vt);
    vterm_state_reset(state, true);

    vts = vterm_obtain_screen(vt);
    vterm_screen_set_callbacks(vts, &callbacks, this);

    vterm_output_set_callback(vt, static_wrap<&TVTermAdapter::writeOutput>, this);

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

void TVTermAdapter::setSize(int rows, int cols)
{
    if (!resizing)
    {
        resizing = true;
        vterm_set_size(vt, rows, cols);
        resizing = false;
    }
}

void TVTermAdapter::handleEvent(TEvent &ev)
{
    bool handled = true;
    switch (ev.what)
    {
        case evKeyDown:
        {
            VTermModifier mod = tv_vterm_conv_mod(ev.keyDown.controlKeyState);
            if (kbCtrlA <= ev.keyDown.keyCode && ev.keyDown.keyCode <= kbCtrlZ)
            {
                ev.keyDown.text[0] = ev.keyDown.keyCode;
                ev.keyDown.textLength = 1;
                mod = VTermModifier(mod & ~VTERM_MOD_CTRL);
            }
            else if (char c = getAltChar(ev.keyDown.keyCode))
            {
                if (c == '\xF0') c = ' '; // Alt+Space.
                ev.keyDown.text[0] = c;
                ev.keyDown.textLength = 1;
                mod = VTermModifier(mod | VTERM_MOD_ALT); // TVision bug?
            }
            if (ev.keyDown.textLength)
            {
                uint32_t c = utf8To32(ev.keyDown.asText());
                vterm_keyboard_unichar(vt, c, mod);
            }
            else if (VTermKey key = tv_vterm_conv_key(ev.keyDown.keyCode))
            {
                vterm_keyboard_key(vt, key, mod);
            }
            break;
        }
        case evMouseDown:
        case evMouseMove:
        case evMouseAuto:
        case evMouseUp:
        case evMouseWheel:
            if (mouseEnabled)
            {
                VTermModifier mod = tv_vterm_conv_mod(ev.mouse.controlKeyState);
                TPoint where = view.makeLocal(ev.mouse.where);
                int button =    (ev.mouse.buttons & mbLeftButton)   ? 1 :
                                (ev.mouse.buttons & mbMiddleButton) ? 2 :
                                (ev.mouse.buttons & mbRightButton)  ? 3 :
                                (ev.mouse.wheel & mwUp)             ? 4 :
                                (ev.mouse.wheel & mwDown)           ? 5 :
                                                                    0 ;
                cerr << "mouseMove(" << where.y << ", " << where.x << ")" << endl;
                vterm_mouse_move(vt, where.y, where.x, mod);
                if (!(ev.what & (evMouseMove | evMouseAuto)))
                    vterm_mouse_button(vt, button, ev.what != evMouseUp, mod);
            }
            break;
        default:
            handled = false;
            break;
    }

    if (handled)
    {
        flushOutput();
        view.clearEvent(ev);
    }
}

void TVTermAdapter::flushOutput()
{
    int rr = ::write(master_fd, outbuf.data(), outbuf.size());
    (void) rr;
    outbuf.resize(0);
}

// These functions are inline because they may only be invoked through
// 'static_wrap'.

inline void TVTermAdapter::writeOutput(const char *data, size_t size)
{
    outbuf.insert(outbuf.end(), data, data + size);
}

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
    switch (prop)
    {
        case VTERM_PROP_TITLE:
            view.window.setTitle(val->string);
            break;
        case VTERM_PROP_MOUSE:
            mouseEnabled = val->boolean;
            break;
        default: break;
    }
    return 0;
}

inline int TVTermAdapter::bell()
{
    return 0;
}

inline int TVTermAdapter::resize(int rows, int cols)
{
    if (!resizing)
    {
        TPoint d = TPoint {cols, rows} - view.size;
        if (d.x || d.y)
        {
            TRect r = view.window.getBounds();
            r.b += d;
            view.window.setBounds(r); // Triggers setSize() -> vterm_set_size() -> resize().
        }
    }
    else
        callbacks.damage({0, rows, 0, cols}, this);
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
