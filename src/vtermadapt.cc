#define Uses_TText
#define Uses_TKeys
#define Uses_TProgram
#include <tvision/tv.h>

#include <tvterm/vtermadapt.h>
#include <tvterm/vtermview.h>
#include <tvterm/vtermwnd.h>
#include <tvterm/ptylisten.h>
#include <tvterm/debug.h>
#include <tvterm/cmds.h>
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
        }
        if (ev.keyDown.textLength)
        {
            uint32_t c = utf8To32(ev.keyDown.getText());
            vterm_keyboard_unichar(vt, c, mod);
        }
        else
        {
            VTermKey key = convKey(ev.keyDown.keyCode);
            if (key)
                vterm_keyboard_key(vt, key, mod);
        }
    }

    template <class OutputState, class Flush>
    static void processMouseDown(TEvent &ev, OutputState &mVT, TView &view, Flush &&flush)
    {
        bool mouseEnabled = false;
        do {
            TPoint where; VTermModifier mod; int button;
            convMouse(ev, view, where, mod, button);
            if (mVT.get().mouseEnabled)
                mVT.lock([&] (auto &vtState) {
                    if ((mouseEnabled = vtState.mouseEnabled))
                    {
                        vterm_mouse_move(vtState.vt, where.y, where.x, mod);
                        if (ev.what & (evMouseDown | evMouseUp | evMouseWheel))
                            vterm_mouse_button(vtState.vt, button, ev.what != evMouseUp, mod);
                        flush();
                    }
                });
        } while (mouseEnabled && view.mouseEvent(ev, evMouse));
        if (ev.what == evMouseUp)
        {
            TPoint where; VTermModifier mod; int button;
            convMouse(ev, view, where, mod, button);
            if (mVT.get().mouseEnabled)
                mVT.lock([&] (auto &vtState) {
                    if (vtState.mouseEnabled)
                    {
                        vterm_mouse_move(vtState.vt, where.y, where.x, mod);
                        vterm_mouse_button(vtState.vt, button, false, mod);
                    }
                });
        }
    }

    static void processMouseOther(TEvent &ev, VTerm *vt, TView &view)
    {
        TPoint where; VTermModifier mod; int button;
        convMouse(ev, view, where, mod, button);
        vterm_mouse_move(vt, where.y, where.x, mod);
        if (ev.what & (evMouseUp | evMouseWheel))
            vterm_mouse_button(vt, button, ev.what != evMouseUp, mod);
    }

    static void wheelToArrow(TEvent &ev, VTerm *vt)
    {
        VTermKey key = VTERM_KEY_NONE;
        switch (ev.mouse.wheel)
        {
            case mwUp: key = VTERM_KEY_UP; break;
            case mwDown: key = VTERM_KEY_DOWN; break;
            case mwLeft: key = VTERM_KEY_LEFT; break;
            case mwRight: key = VTERM_KEY_RIGHT; break;
        }
        for (int i = 0; i < 3; ++i)
            vterm_keyboard_key(vt, key, VTERM_MOD_NONE);
    }

} // namespace vterminput

namespace vtermoutput
{

    static TColorRGB VTermRGBtoRGB(VTermColor c)
    {
        return {c.rgb.red, c.rgb.green, c.rgb.blue};
    }

    static void convAttr( TColorAttr &dst,
                          const VTermScreenCell &cell )
    {
        auto &vt_fg = cell.fg,
             &vt_bg = cell.bg;
        auto &vt_attr = cell.attrs;
        TColorDesired fg, bg;
        // I prefer '{}', but GCC doesn't optimize it very well.
        memset(&fg, 0, sizeof(fg));
        memset(&bg, 0, sizeof(bg));

        if (!VTERM_COLOR_IS_DEFAULT_FG(&vt_fg))
        {
            if (VTERM_COLOR_IS_INDEXED(&vt_fg))
                fg = TColorXTerm(vt_fg.indexed.idx);
            else if (VTERM_COLOR_IS_RGB(&vt_fg))
                fg = VTermRGBtoRGB(vt_fg);
        }

        if (!VTERM_COLOR_IS_DEFAULT_BG(&vt_bg))
        {
            if (VTERM_COLOR_IS_INDEXED(&vt_bg))
                bg = TColorXTerm(vt_bg.indexed.idx);
            else if (VTERM_COLOR_IS_RGB(&vt_bg))
                bg = VTermRGBtoRGB(vt_bg);
        }

        ushort style =
              (slBold & -!!vt_attr.bold)
            | (slItalic & -!!vt_attr.italic)
            | (slUnderline & -!!vt_attr.underline)
            | (slBlink & -!!vt_attr.blink)
            | (slReverse & -!!vt_attr.reverse)
            | (slStrike & -!!vt_attr.strike)
            ;
        dst = {fg, bg, style};
    }

    static void convText( TSpan<TScreenCell> cells, int x,
                          const VTermScreenCell &vtCell )
    {
        if (vtCell.chars[0] == (uint32_t) -1) // Wide char trail.
        {
            // Turbo Vision and libvterm may disagree on what characters
            // are double-width. If libvterm considers a character isn't
            // double-width but Turbo Vision does, it will manage to display it
            // properly anyway. But, in the opposite case, we need to place a
            // space after the double-width character.
            if (x > 0 && !cells[x - 1].wide)
            {
                ::setChar(cells[x], ' ');
                ::setAttr(cells[x], cells[x - 1].attr);
            }
        }
        else
        {
            size_t length = 0;
            while (vtCell.chars[length])
                ++length;
            TSpan<const uint32_t> text {vtCell.chars, max<size_t>(1, length)};
            size_t ci = x, ti = 0;
            while (TText::eat(cells, ci, text, ti))
                ;
        }

    }

} // namespace vtermoutput

void TVTermAdapter::childActions()
{
    setenv("TERM", "xterm-256color", 1);
    setenv("COLORTERM", "truecolor", 1);
}

TVTermAdapter::TVTermAdapter(TVTermView &view, asio::io_context &io) :
    view(view),
    pty(view.size, childActions),
    listener(*this, io, pty.getMaster()),
    strand(io)
{
    auto *vt = vterm_new(view.size.y, view.size.x);
    vterm_set_utf8(vt, 1);

    auto *state = vterm_obtain_state(vt);
    vterm_state_reset(state, true);

    auto *vts = vterm_obtain_screen(vt);
    vterm_screen_enable_altscreen(vts, true);
    vterm_screen_set_callbacks(vts, &callbacks, this);
    vterm_screen_set_damage_merge(vts, VTERM_DAMAGE_SCROLL);
    vterm_screen_reset(vts, true);

    vterm_output_set_callback(vt, static_wrap<&TVTermAdapter::writeOutput>, this);

    mVT.get().vt = vt;
    mVT.get().state = state;
    mVT.get().vts = vts;
    mDisplay.get().surface.resize(view.size);
    listener.start();
}

TVTermAdapter::~TVTermAdapter()
{
    listener.stop();
    vterm_free(mVT.get().vt);
}

template <class Func>
void TVTermAdapter::sendVT(Func &&func)
{
    asio::dispatch(strand, [mAlive=listener.mAlive, func=std::move(func)] () mutable {
        mAlive->lock([&] (auto &alive) {
            if (alive)
                func();
        });
    });
}

void TVTermAdapter::readInput(TSpan<const char> buf)
{
    mVT.lock([&] (auto &vtState) {
        vterm_input_write(vtState.vt, buf.data(), buf.size());
    });
}

void TVTermAdapter::flushDamage()
{
    mVT.lock([&] (auto &vtState) {
        mDisplay.lock([&] (auto &) {
            vterm_screen_flush_damage(vtState.vts);
        });
    });
}

void TVTermAdapter::handleEvent(TEvent &ev)
{
    using namespace vterminput;

    bool handled = true;
    switch (ev.what)
    {
        case evKeyDown:
        {
            sendVT([&, ev] () mutable {
                mVT.lock([&] (auto &vtState) {
                    processKey(ev, vtState.vt);
                });
            });
            break;
        }
        case evMouseDown:
            processMouseDown(ev, mVT, view, [this] { flushOutput(); });
            break;
        case evMouseWheel:
        case evMouseMove:
        case evMouseAuto:
        case evMouseUp:
            sendVT([&, ev] () mutable {
                mVT.lock([&] (auto &vtState) {
                    if (ev.what == evMouseWheel && !vtState.mouseEnabled && vtState.altScreenEnabled)
                        wheelToArrow(ev, vtState.vt);
                    else if (vtState.mouseEnabled)
                        processMouseOther(ev, vtState.vt, view);
                });
            });
            break;
        case evBroadcast:
            if (ev.message.command == cmIdle)
                idle();
            break;
        default:
            handled = false;
            break;
    }

    if (handled)
    {
        sendVT([&] {
            mVT.lock([&] (auto &) {
                flushOutput();
            });
        });
        if (ev.what != evBroadcast)
            view.clearEvent(ev);
    }
}

void TVTermAdapter::idle()
{
    if (state & vtClosed)
    {
        pty.close();
        state &= ~vtClosed;
    }
    if (state & vtUpdated)
    {
        view.drawView();
        state &= ~vtUpdated;
    }
    if (state & vtTitleSet)
    {
        mVT.lock([&] (auto &vtState) {
            auto &title = vtState.strFragBuf;
            view.window.setTitle({title.data(), title.size()});
        });
        state &= ~vtTitleSet;
    }
}

void TVTermAdapter::flushOutput()
// Pre: mVT is locked.
{
    // Writing to pty could also be done asynchronously via asio, but we don't
    // need that yet.
    auto &writeBuf = mVT.get().writeBuf;
    pty.setBlocking(true);
    pty.write({writeBuf.data(), writeBuf.size()});
    pty.setBlocking(false);
    writeBuf.resize(0);
}

void TVTermAdapter::updateChildSize()
{
    TPoint s = view.size;
    // Signal the child.
    setChildSize(s);
    sendVT([&, s] {
        mVT.lock([&] (auto &vtState) {
            mDisplay.lock([&] (auto &dState) {
                // We ensure that TVTermView never sees a blank surface
                // by re-painting before changing vterm's internal state.
                dState.surface.resize(s);
                damageAll();
            });
            vterm_set_size_safe(vtState.vt, s.y, s.x);
        });
    });
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
    // TODO: Redesign this feature.
//     TPoint d = s - view.size;
//     if (d.x || d.y)
//     {
//         TRect r = view.window.getBounds();
//         r.b += d;
//         view.window.locate(r);
//         vterm_set_size_safe(vt, s.y, s.x);
//         vterm_screen_flush_damage(vts);
//     }
}

void TVTermAdapter::damageAll()
// Pre: mVT and mDisplay are locked
{
    // Calling 'vterm_screen_flush_damage' instead of 'damage' would clear
    // the window after a resize, causing blinking.
    TPoint s;
    vterm_get_size(mVT.get().vt, &s.y, &s.x);
    damage({0, s.y, 0, s.x});
}

void TVTermAdapter::writeOutput(const char *data, size_t size)
// Pre: mVT is locked.
{
    auto &writeBuf = mVT.get().writeBuf;
    writeBuf.insert(writeBuf.end(), data, data + size);
}

int TVTermAdapter::damage(VTermRect rect)
// Pre: mVT and mDisplay are locked.
{
    using namespace vtermoutput;
        auto &vts = mVT.get().vts;
        auto &surface = mDisplay.get().surface;
        TRect r(rect.start_col, rect.start_row, rect.end_col, rect.end_row);
        r.intersect(TRect({0, 0}, surface.size));
        for (int y = r.a.y; y < r.b.y; ++y)
        {
            TSpan<TScreenCell> cells(&surface.at(y, 0), surface.size.x);
            for (int x = r.a.x; x < r.b.x; ++x)
            {
                VTermScreenCell cell;
                if (vterm_screen_get_cell(vts, {y, x}, &cell))
                {
                    convAttr(cells[x].attr, cell);
                    convText(cells, x, cell);
                }
                else
                {
                    // The size in 'vt' and 'vts' is sometimes mismatched and
                    // the cell access fails.
                    cells[x] = {};
                }
            }
            auto &damage = surface.rowDamage[y];
            damage.begin = min(r.a.x, damage.begin);
            damage.end = max(r.b.x, damage.end);
        }
    return true;
}

int TVTermAdapter::moverect(VTermRect dest, VTermRect src)
// Pre: mVT is locked.
{
    dout << "moverect(" << dest << ", " << src << ")" << endl;
    return false;
}

int TVTermAdapter::movecursor(VTermPos pos, VTermPos oldpos, int visible)
// Pre: mVT is locked.
{
    mDisplay.lock([&] (auto &dState) {
        dState.cursorChanged = true;
        dState.cursorPos = {pos.col, pos.row};
    });
    return true;
}

int TVTermAdapter::settermprop(VTermProp prop, VTermValue *val)
// Pre: mVT is locked.
{
    dout << "settermprop(" << prop << ", " << val << ")" << endl;
    auto &strFragBuf = mVT.get().strFragBuf;
    if (vterm_get_prop_type(prop) == VTERM_VALUETYPE_STRING)
    {
        if (val->string.initial)
            strFragBuf.resize(0);
        strFragBuf.insert(strFragBuf.end(), val->string.str, val->string.str + val->string.len);
        if (!val->string.final)
            return true;
    }

    switch (prop)
    {
        case VTERM_PROP_TITLE:
            state |= vtTitleSet;
            TEventQueue::wakeUp();
            break;
        case VTERM_PROP_CURSORVISIBLE:
            mDisplay.lock([&] (auto &dState) {
                dState.cursorChanged = true;
                dState.cursorVisible = val->boolean;
            });
            break;
        case VTERM_PROP_CURSORBLINK:
            mDisplay.lock([&] (auto &dState) {
                dState.cursorChanged = true;
                dState.cursorBlink = val->boolean;
            });
            break;
        case VTERM_PROP_MOUSE:
            mVT.get().mouseEnabled = val->boolean;
            break;
        case VTERM_PROP_ALTSCREEN:
            mVT.get().altScreenEnabled = val->boolean;
            break;
        default:
            return false;
    }
    return true;
}

int TVTermAdapter::bell()
// Pre: mVT is locked.
{
    dout << "bell()" << endl;
    return false;
}

int TVTermAdapter::resize(int rows, int cols)
// Pre: mVT is locked.
{
    return false;
}

int TVTermAdapter::sb_pushline(int cols, const VTermScreenCell *cells)
// Pre: mVT is locked.
{
    mVT.get().linestack.push(std::max(cols, 0), cells);
    return true;
}

int TVTermAdapter::sb_popline(int cols, VTermScreenCell *cells)
// Pre: mVT is locked.
{
    return mVT.get().linestack.pop(*this, std::max(cols, 0), cells);
}

void TVTermAdapter::LineStack::push(size_t cols, const VTermScreenCell *src)
{
    if (stack.size() < maxSize)
    {
        auto line = TSpan(new VTermScreenCell[cols], cols);
        memcpy(line.data(), src, line.size_bytes());
        stack.emplace_back(line.data(), cols);
    }
}

bool TVTermAdapter::LineStack::pop( const TVTermAdapter &vterm,
                                    size_t cols, VTermScreenCell *dst )
// Pre: mVT is locked.
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
