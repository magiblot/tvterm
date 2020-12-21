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
#include <algorithm>

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

namespace vterminput
{

    static const std::unordered_map<ushort, ushort> keys =
    {
        { kbEnter,          VTERM_KEY_ENTER             },
        { kbCtrlEnter,      VTERM_KEY_ENTER             },
        { kbTab,            VTERM_KEY_TAB               },
        { kbShiftTab,       VTERM_KEY_TAB               },
        { kbBack,           VTERM_KEY_BACKSPACE         },
        { kbAltBack,        VTERM_KEY_BACKSPACE         },
        { kbCtrlBack,       VTERM_KEY_BACKSPACE         },
        { kbEsc,            VTERM_KEY_ESCAPE            },
        { kbUp,             VTERM_KEY_UP                },
        { kbAltUp,          VTERM_KEY_UP                },
        { kbCtrlUp,         VTERM_KEY_UP                },
        { kbDown,           VTERM_KEY_DOWN              },
        { kbAltDown,        VTERM_KEY_DOWN              },
        { kbCtrlDown,       VTERM_KEY_DOWN              },
        { kbLeft,           VTERM_KEY_LEFT              },
        { kbAltLeft,        VTERM_KEY_LEFT              },
        { kbCtrlLeft,       VTERM_KEY_LEFT              },
        { kbRight,          VTERM_KEY_RIGHT             },
        { kbAltRight,       VTERM_KEY_RIGHT             },
        { kbCtrlRight,      VTERM_KEY_RIGHT             },
        { kbIns,            VTERM_KEY_INS               },
        { kbAltIns,         VTERM_KEY_INS               },
        { kbCtrlIns,        VTERM_KEY_INS               },
        { kbShiftIns,       VTERM_KEY_INS               },
        { kbDel,            VTERM_KEY_DEL               },
        { kbAltDel,         VTERM_KEY_DEL               },
        { kbCtrlDel,        VTERM_KEY_DEL               },
        { kbShiftDel,       VTERM_KEY_DEL               },
        { kbHome,           VTERM_KEY_HOME              },
        { kbAltHome,        VTERM_KEY_HOME              },
        { kbCtrlHome,       VTERM_KEY_HOME              },
        { kbEnd,            VTERM_KEY_END               },
        { kbAltEnd,         VTERM_KEY_END               },
        { kbCtrlEnd,        VTERM_KEY_END               },
        { kbPgUp,           VTERM_KEY_PAGEUP            },
        { kbAltPgUp,        VTERM_KEY_PAGEUP            },
        { kbCtrlPgUp,       VTERM_KEY_PAGEUP            },
        { kbPgDn,           VTERM_KEY_PAGEDOWN          },
        { kbAltPgDn,        VTERM_KEY_PAGEDOWN          },
        { kbCtrlPgDn,       VTERM_KEY_PAGEDOWN          },
        { kbF1,             VTERM_KEY_FUNCTION(1)       },
        { kbF2,             VTERM_KEY_FUNCTION(2)       },
        { kbF3,             VTERM_KEY_FUNCTION(3)       },
        { kbF4,             VTERM_KEY_FUNCTION(4)       },
        { kbF5,             VTERM_KEY_FUNCTION(5)       },
        { kbF6,             VTERM_KEY_FUNCTION(6)       },
        { kbF7,             VTERM_KEY_FUNCTION(7)       },
        { kbF8,             VTERM_KEY_FUNCTION(8)       },
        { kbF9,             VTERM_KEY_FUNCTION(9)       },
        { kbF10,            VTERM_KEY_FUNCTION(10)      },
        { kbF11,            VTERM_KEY_FUNCTION(11)      },
        { kbF12,            VTERM_KEY_FUNCTION(12)      },
        { kbShiftF1,        VTERM_KEY_FUNCTION(1)       },
        { kbShiftF2,        VTERM_KEY_FUNCTION(2)       },
        { kbShiftF3,        VTERM_KEY_FUNCTION(3)       },
        { kbShiftF4,        VTERM_KEY_FUNCTION(4)       },
        { kbShiftF5,        VTERM_KEY_FUNCTION(5)       },
        { kbShiftF6,        VTERM_KEY_FUNCTION(6)       },
        { kbShiftF7,        VTERM_KEY_FUNCTION(7)       },
        { kbShiftF8,        VTERM_KEY_FUNCTION(8)       },
        { kbShiftF9,        VTERM_KEY_FUNCTION(9)       },
        { kbShiftF10,       VTERM_KEY_FUNCTION(10)      },
        { kbShiftF11,       VTERM_KEY_FUNCTION(11)      },
        { kbShiftF12,       VTERM_KEY_FUNCTION(12)      },
        { kbCtrlF1,         VTERM_KEY_FUNCTION(1)       },
        { kbCtrlF2,         VTERM_KEY_FUNCTION(2)       },
        { kbCtrlF3,         VTERM_KEY_FUNCTION(3)       },
        { kbCtrlF4,         VTERM_KEY_FUNCTION(4)       },
        { kbCtrlF5,         VTERM_KEY_FUNCTION(5)       },
        { kbCtrlF6,         VTERM_KEY_FUNCTION(6)       },
        { kbCtrlF7,         VTERM_KEY_FUNCTION(7)       },
        { kbCtrlF8,         VTERM_KEY_FUNCTION(8)       },
        { kbCtrlF9,         VTERM_KEY_FUNCTION(9)       },
        { kbCtrlF10,        VTERM_KEY_FUNCTION(10)      },
        { kbCtrlF11,        VTERM_KEY_FUNCTION(11)      },
        { kbCtrlF12,        VTERM_KEY_FUNCTION(12)      },
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
            return VTermKey(it->second);
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

    static void convMouse( TEvent &ev, TView &view,
                           TPoint &where, VTermModifier &mod, int &button )
    {
        {
            TPoint p = view.makeLocal(ev.mouse.where);
            where = {
                std::clamp(p.x, 0, view.size.x),
                std::clamp(p.y, 0, view.size.y),
            };
        }
        mod = convMod(ev.mouse.controlKeyState);
        button =    (ev.mouse.buttons & mbLeftButton)   ? 1 :
                    (ev.mouse.buttons & mbMiddleButton) ? 2 :
                    (ev.mouse.buttons & mbRightButton)  ? 3 :
                    (ev.mouse.wheel & mwUp)             ? 4 :
                    (ev.mouse.wheel & mwDown)           ? 5 :
                                                          0 ;
    }

    static void processKey(TEvent &ev, VTerm *vt)
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
        else
        {
            VTermKey key = convKey(ev.keyDown.keyCode);
            if (key)
                vterm_keyboard_key(vt, key, mod);
        }
    }

    template <class Flush>
    static void processMouseDown(TEvent &ev, VTerm *vt, TView &view, Flush &&flush)
    {
        do {
            TPoint where; VTermModifier mod; int button;
            convMouse(ev, view, where, mod, button);
            vterm_mouse_move(vt, where.y, where.x, mod);
            if (ev.what & (evMouseDown | evMouseUp | evMouseWheel))
                vterm_mouse_button(vt, button, ev.what != evMouseUp, mod);
            flush();
        } while (view.mouseEvent(ev, evMouse));
        if (ev.what == evMouseUp)
        {
            TPoint where; VTermModifier mod; int button;
            convMouse(ev, view, where, mod, button);
            vterm_mouse_move(vt, where.y, where.x, mod);
            vterm_mouse_button(vt, button, false, mod);
        }
    }

    static void processMouseOther(TEvent &ev, VTerm *vt, TView &view)
    {
        TPoint where; VTermModifier mod; int button;
        convMouse(ev, view, where, mod, button);
        vterm_mouse_move(vt, where.y, where.x, mod);
        if (ev.what == evMouseWheel)
            vterm_mouse_button(vt, button, true, mod);
    }

} // namespace vterminput

namespace vtermoutput
{

    static void convAttr( TScreenCell &dst,
                          const VTermScreenCell &cell )
    {
        TCellAttribs attr {};
        auto fg = cell.fg,
             bg = cell.bg;
        if ( VTERM_COLOR_IS_DEFAULT_FG(&fg) ||
             fg.type != VTERM_COLOR_INDEXED )
        {
            attr.fgSet(0x7); // This shouldn't be necessary. TVision FIXME.
            attr.fgDefault = 1;
        }
        else
            attr.fgSet(swapRedBlue(fg.indexed.idx));
        if ( VTERM_COLOR_IS_DEFAULT_BG(&bg) ||
             bg.type != VTERM_COLOR_INDEXED )
        {
            attr.bgSet(0x0);
            attr.bgDefault = 1;
        }
        else
            attr.bgSet(swapRedBlue(bg.indexed.idx));
        attr.bold = cell.attrs.bold;
        attr.underline = !!cell.attrs.underline;
        attr.italic = cell.attrs.italic;
        attr.reverse = cell.attrs.reverse;
        ::setAttr(dst, attr);
    }

    static void convText( TSpan<TScreenCell> cells, size_t &ci,
                          VTermScreen *vts, TPoint pos )
    {
        char text[4*VTERM_MAX_CHARS_PER_CELL];
        VTermRect rect = {pos.y, pos.y + 1, pos.x, pos.x + 1};
        size_t textLength = vterm_screen_get_text(vts, text, sizeof(text), rect);
        if (!textLength)
        {
            text[0] = '\0';
            textLength = 1;
        }
        size_t ti = 0;
        while (TText::eat(cells, ci, {text, textLength}, ti))
            ;
    }

} // namespace vtermoutput

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
    vterm_screen_enable_altscreen(vts, true);
    vterm_screen_set_callbacks(vts, &callbacks, this);

    vterm_output_set_callback(vt, static_wrap<&TVTermAdapter::writeOutput>, this);

    pty = PTY::create(view.size,
        []() {
            setenv("TERM", "xterm-16color", 1);
            unsetenv("COLORTERM");
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
    using namespace vterminput;

    bool handled = true;
    switch (ev.what)
    {
        case evKeyDown:
        {
            processKey(ev, vt);
            break;
        }
        case evMouseDown:
            if (mouseEnabled)
                processMouseDown(ev, vt, view, [this] { flushOutput(); });
            break;
        case evMouseMove:
        case evMouseAuto:
        case evMouseWheel:
            if (mouseEnabled)
                processMouseOther(ev, vt, view);
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
    // This function is usually invoked during resizing.
    // We rely on TVTermView::draw() being invoked at some point after this.
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

void TVTermAdapter::damageAll()
{
    // Redrawing this way reduces flicker.
    // 'vterm_screen_flush_damage' would clear the window after
    // a resize.
    TPoint s;
    vterm_get_size(vt, &s.y, &s.x);
    damage({0, s.y, 0, s.x});
}

void TVTermAdapter::writeOutput(const char *data, size_t size)
{
    outbuf.insert(outbuf.end(), data, data + size);
}

int TVTermAdapter::damage(VTermRect rect)
{
    using namespace vtermoutput;
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
                convAttr(cells[ci], cell);
                convText(cells, ci, vts, {x, y});
                x += cell.width;
            }
        }
        return true;
    }
    else
    {
        dout << pty.getMaster() << ": no draw buffer!" << endl;
        return false;
    }
}

int TVTermAdapter::moverect(VTermRect dest, VTermRect src)
{
    dout << "moverect(" << dest << ", " << src << ")" << endl;
    return false;
}

int TVTermAdapter::movecursor(VTermPos pos, VTermPos oldpos, int visible)
{
    updateParentSize();
    view.setCursor(pos.col, pos.row);
    return true;
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
        default:
            return false;
    }
    return true;
}

int TVTermAdapter::bell()
{
    dout << "bell()" << endl;
    return false;
}

int TVTermAdapter::resize(int rows, int cols)
{
    return false;
}

int TVTermAdapter::sb_pushline(int cols, const VTermScreenCell *cells)
{
    linestack.push(std::max(cols, 0), cells);
    return true;
}

int TVTermAdapter::sb_popline(int cols, VTermScreenCell *cells)
{
    return linestack.pop(*this, std::max(cols, 0), cells);
}

void TVTermAdapter::LineStack::push(size_t cols, const VTermScreenCell *src)
{
    auto line = TSpan(new VTermScreenCell[cols], cols);
    memcpy(line.data(), src, line.size_bytes());
    stack.emplace_back(line.data(), cols);
}

bool TVTermAdapter::LineStack::pop( const TVTermAdapter &vterm,
                                    size_t cols, VTermScreenCell *dst )
{
    if (!stack.empty())
    {
        auto line = top();
        size_t dst_size = cols*sizeof(VTermScreenCell);
        size_t copy_bytes = std::min(line.size_bytes(), dst_size);
        memcpy(dst, line.data(), copy_bytes);
        auto cell = vterm.getDefaultCell();
        for (size_t i = line.size(); i < cols; ++i)
            dst[i] = cell;
        stack.pop_back();
        return true;
    }
    return false;
}
