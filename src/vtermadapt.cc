#define Uses_TText
#define Uses_TKeys
#include <tvision/tv.h>

#include <tvterm/vtermadapt.h>
#include <tvterm/vtermview.h>
#include <tvterm/vtermwnd.h>
#include <tvterm/ptylisten.h>
#include <tvterm/debug.h>
#include <tvterm/pty.h>
#include <unordered_map>

const VTermScreenCallbacks TVTermAdapter::callbacks =
{
    static_wrap<&TVTermAdapter::damage>,
    static_wrap<&TVTermAdapter::moverect>,
    static_wrap<&TVTermAdapter::movecursor>,
    static_wrap<&TVTermAdapter::settermprop>,
    static_wrap<&TVTermAdapter::bell>,
    static_wrap<&TVTermAdapter::resize>,
    static_wrap<&TVTermAdapter::sb_pushline>,
    static_wrap<&TVTermAdapter::sb_popline>,
};

namespace vtermadapt
{

    static const std::unordered_map<ushort, VTermKey> keys =
    {
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

    static constexpr struct { ushort tvmod; VTermModifier vtmod; } modifiers[] =
    {
        { kbShift,      VTERM_MOD_SHIFT },
        { kbAltShift,   VTERM_MOD_ALT   },
        { kbCtrlShift,  VTERM_MOD_CTRL  },
    };

    static VTermKey convKey(ushort keyCode)
    {
        auto it = keys.find(keyCode);
        if (it != keys.end())
            return it->second;
        return VTERM_KEY_NONE;
    }

    static VTermModifier convMod(ulong controlKeyState)
    {
        VTermModifier mod = VTERM_MOD_NONE;
        for (const auto [tvmod, vtmod] : modifiers)
            if (controlKeyState & tvmod)
                mod = VTermModifier(mod | vtmod);
        return mod;
    }

} // namespace vtermadapt

TVTermAdapter::TVTermAdapter(TVTermView &view) :
    view(view),
    listener(*this),
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

    pty = PTY::create(view.size,
        []() {
            setenv("TERM", "xterm", 1);
        }
    );

    listener.listen(pty.getMaster());
}

TVTermAdapter::~TVTermAdapter()
{
    listener.disconnect();
    pty.destroy();
    vterm_free(vt);
}

void TVTermAdapter::read()
{
    if (pending)
    {
        pending = false;
        updateParentSize();
        pty.setBlocking(false);
        char buf[4096];
        ssize_t size;
        while ((size = pty.read(buf)) > 0)
        {
            dout << pty.getMaster() << ": read " << size << " bytes." << endl;
            vterm_input_write(vt, buf, size);
        }
        updateParentSize();
        vterm_screen_flush_damage(vts);
        view.window.drawView();
    }
}

void TVTermAdapter::handleEvent(TEvent &ev)
{
    using namespace vtermadapt;

    bool handled = true;
    switch (ev.what)
    {
        case evKeyDown:
        {
            VTermModifier mod = convMod(ev.keyDown.controlKeyState);
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
            else if (VTermKey key = convKey(ev.keyDown.keyCode))
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
                VTermModifier mod = convMod(ev.mouse.controlKeyState);
                TPoint where = view.makeLocal(ev.mouse.where);
                int button =    (ev.mouse.buttons & mbLeftButton)   ? 1 :
                                (ev.mouse.buttons & mbMiddleButton) ? 2 :
                                (ev.mouse.buttons & mbRightButton)  ? 3 :
                                (ev.mouse.wheel & mwUp)             ? 4 :
                                (ev.mouse.wheel & mwDown)           ? 5 :
                                                                      0 ;
                dout << "mouseMove(" << where.y << ", " << where.x << ")" << endl;
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
    pty.write({outbuf.data(), outbuf.size()});
    outbuf.resize(0);
}

void TVTermAdapter::updateChildSize()
{
    TPoint s = view.size;
    // Signal the child.
    setChildSize(s);
    vterm_set_size_safe(vt, s.y, s.x);
    // Redrawing this way reduces flicker.
    // 'vterm_screen_flush_damage' would clear the window.
    callbacks.damage({0, s.y, 0, s.x}, this);
}

void TVTermAdapter::updateParentSize()
{
    setParentSize(getChildSize());
}

TPoint TVTermAdapter::getChildSize() const
{
    TPoint s;
    pty.getSize(s);
    return s;
}

void TVTermAdapter::setChildSize(TPoint s) const
{
    if (s != getChildSize())
        pty.setSize(s);
}

void TVTermAdapter::setParentSize(TPoint s)
{
    TPoint d = s - view.size;
    if (d.x || d.y)
    {
        TRect r = view.window.getBounds();
        r.b += d;
        view.window.locate(r);
        vterm_set_size_safe(vt, s.y, s.x);
        vterm_screen_flush_damage(vts);
    }
}

void TVTermAdapter::writeOutput(const char *data, size_t size)
{
    outbuf.insert(outbuf.end(), data, data + size);
}

int TVTermAdapter::damage(VTermRect rect)
{
    if (view.owner->buffer)
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
                    if (!textLength)
                    {
                        text[0] = '\0';
                        textLength = 1;
                    }
                    size_t ti = 0;
                    while (TText::eat(cells, ci, {text, textLength}, ti))
                        ;
                }
                x += cell.width;
            }
        }
    }
    else
    {
        dout << pty.getMaster() << ": no draw buffer!" << endl;
    }
    return 0;
}

int TVTermAdapter::moverect(VTermRect dest, VTermRect src)
{
    dout << "moverect(" << dest << ", " << src << ")" << endl;
    return 0;
}

int TVTermAdapter::movecursor(VTermPos pos, VTermPos oldpos, int visible)
{
    updateParentSize();
    view.setCursor(pos.col, pos.row);
    return 0;
}

int TVTermAdapter::settermprop(VTermProp prop, VTermValue *val)
{
    dout << "settermprop(" << prop << ", " << val << ")" << endl;
    switch (prop)
    {
        case VTERM_PROP_TITLE:
            view.window.setTitle(val->string);
            break;
        case VTERM_PROP_CURSORVISIBLE:
            val->boolean ? view.showCursor() : view.hideCursor();
            break;
        case VTERM_PROP_CURSORBLINK:
            view.setState(sfCursorIns, val->boolean);
            break;
        case VTERM_PROP_MOUSE:
            mouseEnabled = val->boolean;
            break;
        default: break;
    }
    return 0;
}

int TVTermAdapter::bell()
{
    dout << "bell()" << endl;
    return 0;
}

int TVTermAdapter::resize(int rows, int cols)
{
    return 0;
}

int TVTermAdapter::sb_pushline(int cols, const VTermScreenCell *cells)
{
    dout << "sb_pushline(" << cols << ", " << cells << ")" << endl;
    return 0;
}

int TVTermAdapter::sb_popline(int cols, VTermScreenCell *cells)
{
    dout << "sb_popline(" << cols << ", " << cells << ")" << endl;
    return 0;
}