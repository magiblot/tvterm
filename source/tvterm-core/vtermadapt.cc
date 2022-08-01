#define Uses_TText
#define Uses_TKeys
#define Uses_TEvent
#include <tvision/tv.h>

#include "util.h"
#include <tvterm/vtermadapt.h>
#include <tvterm/debug.h>
#include <unordered_map>

namespace tvterm
{

const VTermScreenCallbacks VTermAdapter::callbacks =
{
    _static_wrap(&VTermAdapter::damage),
    _static_wrap(&VTermAdapter::moverect),
    _static_wrap(&VTermAdapter::movecursor),
    _static_wrap(&VTermAdapter::settermprop),
    _static_wrap(&VTermAdapter::bell),
    _static_wrap(&VTermAdapter::resize),
    _static_wrap(&VTermAdapter::sb_pushline),
    _static_wrap(&VTermAdapter::sb_popline),
};

namespace vtermadapt
{

    // Input conversion.

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
        { kbAltF1,          VTERM_KEY_FUNCTION(1)       },
        { kbAltF2,          VTERM_KEY_FUNCTION(2)       },
        { kbAltF3,          VTERM_KEY_FUNCTION(3)       },
        { kbAltF4,          VTERM_KEY_FUNCTION(4)       },
        { kbAltF5,          VTERM_KEY_FUNCTION(5)       },
        { kbAltF6,          VTERM_KEY_FUNCTION(6)       },
        { kbAltF7,          VTERM_KEY_FUNCTION(7)       },
        { kbAltF8,          VTERM_KEY_FUNCTION(8)       },
        { kbAltF9,          VTERM_KEY_FUNCTION(9)       },
        { kbAltF10,         VTERM_KEY_FUNCTION(10)      },
        { kbAltF11,         VTERM_KEY_FUNCTION(11)      },
        { kbAltF12,         VTERM_KEY_FUNCTION(12)      },
    };

    static constexpr struct { ushort tv; VTermModifier vt; } modifiers[] =
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
        for (const auto &m : modifiers)
            if (controlKeyState & m.tv)
                mod = VTermModifier(mod | m.vt);
        return mod;
    }

    static void convMouse( const MouseEventType &mouse,
                           VTermModifier &mod, int &button )
    {
        mod = convMod(mouse.controlKeyState);
        button =    (mouse.buttons & mbLeftButton)   ? 1 :
                    (mouse.buttons & mbMiddleButton) ? 2 :
                    (mouse.buttons & mbRightButton)  ? 3 :
                    (mouse.wheel & mwUp)             ? 4 :
                    (mouse.wheel & mwDown)           ? 5 :
                                                       0 ;
    }

    static void processKey(VTerm *vt, KeyDownEvent keyDown)
    {
        VTermModifier mod = convMod(keyDown.controlKeyState);
        if (kbCtrlA <= keyDown.keyCode && keyDown.keyCode <= kbCtrlZ)
        {
            keyDown.text[0] = keyDown.keyCode;
            keyDown.textLength = 1;
            mod = VTermModifier(mod & ~VTERM_MOD_CTRL);
        }
        else if (char c = getAltChar(keyDown.keyCode))
        {
            if (c == '\xF0') c = ' '; // Alt+Space.
            keyDown.text[0] = c;
            keyDown.textLength = 1;
        }

        if (keyDown.textLength)
            vterm_keyboard_unichar(vt, utf8To32(keyDown.getText()), mod);
        else if (VTermKey key = convKey(keyDown.keyCode))
            vterm_keyboard_key(vt, key, mod);
    }

    static void processMouse(VTerm *vt, ushort what, const MouseEventType &mouse)
    {
        VTermModifier mod; int button;
        convMouse(mouse, mod, button);
        vterm_mouse_move(vt, mouse.where.y, mouse.where.x, mod);
        if (what & (evMouseDown | evMouseUp | evMouseWheel))
            vterm_mouse_button(vt, button, what != evMouseUp, mod);
    }

    static void wheelToArrow(VTerm *vt, uchar wheel)
    {
        VTermKey key = VTERM_KEY_NONE;
        switch (wheel)
        {
            case mwUp: key = VTERM_KEY_UP; break;
            case mwDown: key = VTERM_KEY_DOWN; break;
            case mwLeft: key = VTERM_KEY_LEFT; break;
            case mwRight: key = VTERM_KEY_RIGHT; break;
        }
        for (int i = 0; i < 3; ++i)
            vterm_keyboard_key(vt, key, VTERM_MOD_NONE);
    }

    // Output conversion.

    static TColorRGB VTermRGBtoRGB(VTermColor c)
    {
        return {c.rgb.red, c.rgb.green, c.rgb.blue};
    }

    static TColorAttr convAttr(const VTermScreenCell &cell)
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
        return {fg, bg, style};
    }

    static void convCell( TSpan<TScreenCell> cells, int x,
                          const VTermScreenCell &vtCell )
    {
        if (vtCell.chars[0] == (uint32_t) -1) // Wide char trail.
        {
            // Turbo Vision and libvterm may disagree on what characters
            // are double-width. If libvterm considers a character isn't
            // double-width but Turbo Vision does, it will manage to display it
            // properly anyway. But, in the opposite case, we need to place a
            // space after the double-width character.
            if (x > 0 && !cells[x - 1].isWide())
            {
                ::setChar(cells[x], ' ');
                ::setAttr(cells[x], ::getAttr(cells[x - 1]));
            }
        }
        else
        {
            size_t length = 0;
            while (vtCell.chars[length])
                ++length;
            TSpan<const uint32_t> text {vtCell.chars, max<size_t>(1, length)};
            TText::drawStr(cells, x, text, 0, convAttr(vtCell));
        }
    }

    static void drawArea( VTermScreen *vts, TPoint termSize, TRect area,
                          TerminalSurface &surface )
    {
        TRect r;
        if (surface.size != termSize)
        {
            surface.resize(termSize);
            r = {{0, 0}, termSize};
        }
        else
            r = area.intersect({{0, 0}, termSize});
        if (0 <= r.a.x && r.a.x < r.b.x && 0 <= r.a.y && r.a.y < r.b.y)
            for (int y = r.a.y; y < r.b.y; ++y)
            {
                TSpan<TScreenCell> cells(&surface.at(y, 0), surface.size.x);
                for (int x = r.a.x; x < r.b.x; ++x)
                {
                    VTermScreenCell cell;
                    if (vterm_screen_get_cell(vts, {y, x}, &cell))
                        convCell(cells, x, cell);
                    else
                        cells[x] = {};
                }
                surface.setDamage(y, r.a.x, r.b.x);
            }
    }

} // namespace vtermadapt

VTermAdapter::VTermAdapter(TPoint size, GrowArray &aOutputBuffer, TerminalSharedState &aSharedState) noexcept :
    outputBuffer(aOutputBuffer)
{
    sharedState = &aSharedState;

    vt = vterm_new(max(size.y, 1), max(size.x, 1));
    vterm_set_utf8(vt, 1);

    state = vterm_obtain_state(vt);
    vterm_state_reset(state, true);

    vts = vterm_obtain_screen(vt);
    vterm_screen_enable_altscreen(vts, true);
    vterm_screen_set_callbacks(vts, &callbacks, this);
    vterm_screen_set_damage_merge(vts, VTERM_DAMAGE_SCROLL);
    vterm_screen_reset(vts, true);

    vterm_output_set_callback(vt, _static_wrap(&VTermAdapter::writeOutput), this);

    // VTerm's cursor blinks by default, but it shouldn't.
    VTermValue val {0};
    vterm_state_set_termprop(state, VTERM_PROP_CURSORBLINK, &val);

    sharedState = nullptr;
}

VTermAdapter::~VTermAdapter()
{
    vterm_free(vt);
}

void VTermAdapter::childActions() noexcept
{
    setenv("TERM", "xterm-256color", 1);
    setenv("COLORTERM", "truecolor", 1);
}

void VTermAdapter::receive(TSpan<const char> buf, TerminalSharedState &aSharedState) noexcept
{
    sharedState = &aSharedState;
    vterm_input_write(vt, buf.data(), buf.size());
    sharedState = nullptr;
}

void VTermAdapter::flushDamage(TerminalSharedState &aSharedState) noexcept
{
    sharedState = &aSharedState;
    vterm_screen_flush_damage(vts);
    sharedState = nullptr;
}

void VTermAdapter::handleKeyDown(const KeyDownEvent &keyDown) noexcept
{
    using namespace vtermadapt;
    processKey(vt, keyDown);
}

void VTermAdapter::handleMouse(ushort what, const MouseEventType &mouse) noexcept
{
    using namespace vtermadapt;
    if (mouseEnabled)
        processMouse(vt, what, mouse);
    else if (altScreenEnabled && what == evMouseWheel)
        wheelToArrow(vt, mouse.wheel);
}

inline TPoint VTermAdapter::getSize() noexcept
{
    TPoint size;
    vterm_get_size(vt, &size.y, &size.x);
    return size;
}

void VTermAdapter::setSize(TPoint size, TerminalSharedState &aSharedState) noexcept
{
    sharedState = &aSharedState;
    size.x = max(size.x, 1);
    size.y = max(size.y, 1);
    if (size != getSize())
        vterm_set_size(vt, size.y, size.x);
    sharedState = nullptr;
}

void VTermAdapter::setFocus(bool focus) noexcept
{
    if (focus)
        vterm_state_focus_in(state);
    else
        vterm_state_focus_out(state);
}

void VTermAdapter::writeOutput(const char *data, size_t size)
{
    outputBuffer.push(data, size);
}

int VTermAdapter::damage(VTermRect rect)
{
    using namespace vtermadapt;
    drawArea( vts, getSize(),
              {rect.start_col, rect.start_row, rect.end_col, rect.end_row},
              sharedState->surface );
    return true;
}

int VTermAdapter::moverect(VTermRect dest, VTermRect src)
{
    dout << "moverect(" << dest << ", " << src << ")" << std::endl;
    return false;
}

int VTermAdapter::movecursor(VTermPos pos, VTermPos oldpos, int visible)
{
    sharedState->cursorChanged = true;
    sharedState->cursorPos = {pos.col, pos.row};
    return true;
}

int VTermAdapter::settermprop(VTermProp prop, VTermValue *val)
{
    dout << "settermprop(" << prop << ", " << val << ")" << std::endl;
    if (vterm_get_prop_type(prop) == VTERM_VALUETYPE_STRING)
    {
#ifdef HAVE_VTERMSTRINGFRAGMENT
        if (val->string.initial)
            strFragBuf.clear();
        strFragBuf.push(val->string.str, val->string.len);
        if (!val->string.final)
            return true;
#else
        strFragBuf.clear();
        strFragBuf.push(val->string, val->string ? strlen(val->string) : 0);
#endif
    }

    switch (prop)
    {
        case VTERM_PROP_TITLE:
            sharedState->titleChanged = true;
            sharedState->title = std::move(strFragBuf);
            break;
        case VTERM_PROP_CURSORVISIBLE:
            sharedState->cursorChanged = true;
            sharedState->cursorVisible = val->boolean;
            break;
        case VTERM_PROP_CURSORBLINK:
            sharedState->cursorChanged = true;
            sharedState->cursorBlink = val->boolean;
            break;
        case VTERM_PROP_MOUSE:
            mouseEnabled = val->boolean;
            break;
        case VTERM_PROP_ALTSCREEN:
            altScreenEnabled = val->boolean;
            break;
        default:
            return false;
    }
    return true;
}

int VTermAdapter::bell()
{
    dout << "bell()" << std::endl;
    return false;
}

int VTermAdapter::resize(int rows, int cols)
{
    return false;
}

int VTermAdapter::sb_pushline(int cols, const VTermScreenCell *cells)
{
    linestack.push(std::max(cols, 0), cells);
    return true;
}

int VTermAdapter::sb_popline(int cols, VTermScreenCell *cells)
{
    return linestack.pop(*this, std::max(cols, 0), cells);
}

inline VTermScreenCell VTermAdapter::getDefaultCell() const
{
    VTermScreenCell cell {};
    cell.width = 1;
    vterm_state_get_default_colors(state, &cell.fg, &cell.bg);
    return cell;
}

void VTermAdapter::LineStack::push(size_t cols, const VTermScreenCell *src)
{
    if (stack.size() < maxSize)
    {
        auto *line = new VTermScreenCell[cols];
        memcpy(line, src, sizeof(VTermScreenCell)*cols);
        stack.emplace_back(line, cols);
    }
}

bool VTermAdapter::LineStack::pop( const VTermAdapter &vterm,
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

} // namespace tvterm
