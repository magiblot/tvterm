#define Uses_TText
#define Uses_TKeys
#define Uses_TEvent
#include <tvision/tv.h>

#include "util.h"
#include <tvterm/vtermemu.h>
#include <tvterm/debug.h>
#include <unordered_map>

namespace tvterm
{

const VTermScreenCallbacks VTermEmulator::callbacks =
{
    _static_wrap(&VTermEmulator::damage),
    _static_wrap(&VTermEmulator::moverect),
    _static_wrap(&VTermEmulator::movecursor),
    _static_wrap(&VTermEmulator::settermprop),
    _static_wrap(&VTermEmulator::bell),
    _static_wrap(&VTermEmulator::resize),
    /* sb_pushline */ nullptr,
    /* sb_popline */ nullptr,
    /* sb_clear */ nullptr,
    _static_wrap(&VTermEmulator::sb_pushline4),
};

namespace vtermemu
{

    // Input conversion.

    static const std::unordered_map<ushort, ushort> keys =
    {
        { kbEnter,          VTERM_KEY_ENTER             },
        { kbTab,            VTERM_KEY_TAB               },
        { kbBack,           VTERM_KEY_BACKSPACE         },
        { kbEsc,            VTERM_KEY_ESCAPE            },
        { kbUp,             VTERM_KEY_UP                },
        { kbDown,           VTERM_KEY_DOWN              },
        { kbLeft,           VTERM_KEY_LEFT              },
        { kbRight,          VTERM_KEY_RIGHT             },
        { kbIns,            VTERM_KEY_INS               },
        { kbDel,            VTERM_KEY_DEL               },
        { kbHome,           VTERM_KEY_HOME              },
        { kbEnd,            VTERM_KEY_END               },
        { kbPgUp,           VTERM_KEY_PAGEUP            },
        { kbPgDn,           VTERM_KEY_PAGEDOWN          },
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
    };

    static constexpr struct { ushort tv; VTermModifier vt; } modifiers[] =
    {
        { kbShift,      VTERM_MOD_SHIFT },
        { kbLeftAlt,    VTERM_MOD_ALT   },
        { kbCtrlShift,  VTERM_MOD_CTRL  },
    };

    static VTermKey convKey(ushort keyCode)
    {
        auto it = keys.find(keyCode);
        if (it != keys.end())
            return VTermKey(it->second);
        return VTERM_KEY_NONE;
    }

    static VTermModifier convMod(ushort controlKeyState)
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
        TKey tvKey(keyDown);
        VTermModifier vtMod = convMod(tvKey.mods);
        // Pass control characters directly, with no modifiers.
        if ( tvKey.mods == kbCtrlShift
             && 'A' <= tvKey.code && tvKey.code <= 'Z' )
        {
            keyDown.text[0] = keyDown.charScan.charCode;
            keyDown.textLength = 1;
            vtMod = VTERM_MOD_NONE;
        }
        // Pass other legacy 'letter+mod' combinations as text.
        else if ( keyDown.textLength == 0
                  && ' ' <= tvKey.code && tvKey.code < '\x7F' )
        {
            keyDown.text[0] = (char) tvKey.code;
            keyDown.textLength = 1;
            // On Windows, ConPTY unfortunately adds the Shift modifier on an
            // uppercase Alt+Key, so make it lowercase.
            if ( (keyDown.controlKeyState & (kbShift | kbCtrlShift | kbAltShift)) == kbLeftAlt
                 && ('A' <= tvKey.code && tvKey.code <= 'Z') )
                keyDown.text[0] += 'a' - 'A';
        }

        if (keyDown.textLength != 0)
            vterm_keyboard_unichar(vt, utf8To32(keyDown.getText()), vtMod);
        else if (VTermKey vtKey = convKey(tvKey.code))
            vterm_keyboard_key(vt, vtKey, vtMod);
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
        for (int i = 0; i < VTermEmulator::mouseWheelScrollLines; ++i)
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

    static void drawLine( TerminalSurface &surface, VTermScreen *vtScreen,
                          int y, int rowIndex, int begin, int end )
    // Pre: the area must be within bounds.
    {
        dout << "drawLine(" << y << ", " << begin << ", " << end << ")" << std::endl;
        TSpan<TScreenCell> cells(&surface.at(y, 0), surface.size.x);
        for (int x = begin; x < end; ++x)
        {
            VTermScreenCell cell;
            if (vterm_screen_get_cell(vtScreen, {rowIndex, x}, &cell))
                convCell(cells, x, cell);
            else
                cells[x] = {};
        }
        surface.addDamageAtRow(y, begin, end);
    }

} // namespace vtermemu

TerminalEmulator &VTermEmulatorFactory::create(TPoint size, Writer &clientDataWriter) noexcept
{
    return *new VTermEmulator(size, clientDataWriter);
}

TSpan<const EnvironmentVar> VTermEmulatorFactory::getCustomEnvironment() noexcept
{
    static constexpr EnvironmentVar customEnvironment[] =
    {
        {"TERM", "xterm-256color"},
        {"COLORTERM", "truecolor"},
    };

    return customEnvironment;
}

VTermEmulator::VTermEmulator(TPoint size, Writer &aClientDataWriter) noexcept :
    clientDataWriter(aClientDataWriter)
{
    // VTerm requires size to be at least 1.
    size.x = max(size.x, 1);
    size.y = max(size.y, 1);
    damageByRow.resize(size.y);

    vt = vterm_new(size.y, size.x);
    vterm_set_utf8(vt, 1);

    vtState = vterm_obtain_state(vt);
    vterm_state_reset(vtState, true);

    vtScreen = vterm_obtain_screen(vt);
    vterm_screen_enable_altscreen(vtScreen, true);
    vterm_screen_set_callbacks(vtScreen, &callbacks, this);
    vterm_screen_set_damage_merge(vtScreen, VTERM_DAMAGE_SCROLL);
    vterm_screen_reset(vtScreen, true);

    vterm_screen_callbacks_has_pushline4(vtScreen);
    // This cannot be enabled yet, it will easily crash (e.g. shrink the terminal window
    // while running an alternate buffer application):
    // vterm_screen_enable_reflow(vtScreen, true);

    vterm_output_set_callback(vt, _static_wrap(&VTermEmulator::writeOutput), this);

    // VTerm's cursor blinks by default, but it shouldn't.
    VTermValue val {0};
    vterm_state_set_termprop(vtState, VTERM_PROP_CURSORBLINK, &val);
}

VTermEmulator::~VTermEmulator()
{
    vterm_free(vt);
}

void VTermEmulator::handleEvent(const TerminalEvent &event) noexcept
{
    using namespace vtermemu;
    switch (event.type)
    {
        case TerminalEventType::KeyDown:
            processKey(vt, event.keyDown);
            scrollback.followTerminal();
            break;

        case TerminalEventType::Mouse:
            if (localState.mouseEnabled)
                processMouse(vt, event.mouse.what, event.mouse.mouse);
            else if (localState.altScreenEnabled && event.mouse.what == evMouseWheel)
                wheelToArrow(vt, event.mouse.mouse.wheel);
            else if (event.mouse.what == evMouseWheel)
                handleScrollbackWheel(event.mouse.mouse.wheel);
            break;

        case TerminalEventType::ClientDataRead:
        {
            auto &clientData = event.clientDataRead;
            vterm_input_write(vt, clientData.data, clientData.size);
            break;
        }

        case TerminalEventType::ViewportResize:
        {
            TPoint size = {event.viewportResize.x, event.viewportResize.y};
            setSize(size);
            break;
        }

        case TerminalEventType::FocusChange:
            if (event.focusChange.focusEnabled)
                vterm_state_focus_in(vtState);
            else
                vterm_state_focus_out(vtState);
            break;

        default:
            break;
    }
}

void VTermEmulator::updateState(TerminalState &state) noexcept
{
    vterm_screen_flush_damage(vtScreen);
    drawDamagedArea(state.surface);
    if (localState.cursorChanged)
    {
        localState.cursorChanged = false;
        state.cursorChanged = true;
        state.cursorPos = localState.cursorPos;
        state.cursorVisible = localState.cursorVisible;
        state.cursorBlink = localState.cursorBlink;
    }
    if (localState.titleChanged)
    {
        localState.titleChanged = false;
        state.titleChanged = true;
        state.title = std::move(localState.title);
    }
}

TPoint VTermEmulator::getSize() noexcept
{
    TPoint size;
    vterm_get_size(vt, &size.y, &size.x);
    return size;
}

void VTermEmulator::setSize(TPoint size) noexcept
{
    size.x = max(size.x, 1);
    size.y = max(size.y, 1);

    if (size != getSize())
    {
        vterm_set_size(vt, size.y, size.x);
        damageByRow.resize(0);
        damageByRow.resize(size.y);
    }
}

void VTermEmulator::drawDamagedArea(TerminalSurface &surface) noexcept
{
    using namespace vtermemu;
    TPoint size = getSize();
    if (surface.size != size)
        surface.resize(size);

    int visibleScrollLines = scrollback.getVisibleScrollLines(size.y);
    bool scrollbackChanged = ( scrollback.offset != localState.scrollOffset ||
                               visibleScrollLines != localState.visibleScrollLines );
    localState.scrollOffset = scrollback.offset;
    localState.visibleScrollLines = visibleScrollLines;

    if (scrollbackChanged)
        for (int y = 0; y < visibleScrollLines; ++y)
            drawScrollbackLine(surface, y);

    for (int y = visibleScrollLines; y < size.y; ++y)
    {
        int rowIndex = y - visibleScrollLines;

        auto &damage = damageByRow[rowIndex];
        int begin = max(damage.begin, 0);
        int end = min(damage.end, size.x);

        if (scrollbackChanged)
        {
            begin = 0;
            end = size.x;
        }

        if (begin < end)
            drawLine(surface, vtScreen, y, rowIndex, begin, end);

        damage = {};
    }
}

void VTermEmulator::writeOutput(const char *data, size_t size)
{
    clientDataWriter.write({data, size});
}

int VTermEmulator::damage(VTermRect rect)
{
    rect.start_row = min(max(rect.start_row, 0), damageByRow.size());
    rect.end_row = min(max(rect.end_row, 0), damageByRow.size());
    for (int y = rect.start_row; y < rect.end_row; ++y)
    {
        auto &damage = damageByRow[y];
        damage.begin = min(rect.start_col, damage.begin);
        damage.end = max(rect.end_col, damage.end);
    }
    return true;
}

int VTermEmulator::moverect(VTermRect dest, VTermRect src)
{
    dout << "moverect(" << dest << ", " << src << ")" << std::endl;
    return false;
}

int VTermEmulator::movecursor(VTermPos pos, VTermPos oldpos, int visible)
{
    localState.cursorChanged = true;
    localState.cursorPos = {pos.col, pos.row};
    return true;
}

int VTermEmulator::settermprop(VTermProp prop, VTermValue *val)
{
    dout << "settermprop(" << prop << ", " << val << ")" << std::endl;
    if (vterm_get_prop_type(prop) == VTERM_VALUETYPE_STRING)
    {
        if (val->string.initial)
            strFragBuf.clear();
        strFragBuf.push(val->string.str, val->string.len);
        if (!val->string.final)
            return true;
    }

    switch (prop)
    {
        case VTERM_PROP_TITLE:
            localState.titleChanged = true;
            localState.title = std::move(strFragBuf);
            break;
        case VTERM_PROP_CURSORVISIBLE:
            localState.cursorChanged = true;
            localState.cursorVisible = val->boolean;
            break;
        case VTERM_PROP_CURSORBLINK:
            localState.cursorChanged = true;
            localState.cursorBlink = val->boolean;
            break;
        case VTERM_PROP_MOUSE:
            localState.mouseEnabled = val->boolean;
            break;
        case VTERM_PROP_ALTSCREEN:
            localState.altScreenEnabled = val->boolean;
            scrollback.followTerminal();
            break;
        default:
            return false;
    }
    return true;
}

int VTermEmulator::bell()
{
    dout << "bell()" << std::endl;
    return false;
}

int VTermEmulator::resize(int rows, int cols)
{
    return false;
}

int VTermEmulator::sb_pushline4(int cols, const VTermScreenCell *cells, bool continuation)
{
    if (!localState.altScreenEnabled)
        scrollback.pushLine(cols, cells, continuation);
    return true;
}

void VTermEmulator::handleScrollbackWheel(uchar wheel) noexcept
{
    if (wheel == mwUp)
        scrollback.incrementOffset(-mouseWheelScrollLines);
    else if (wheel == mwDown)
        scrollback.incrementOffset(mouseWheelScrollLines);
}

void VTermEmulator::drawScrollbackLine(TerminalSurface &surface, int y) noexcept
{
    using namespace vtermemu;
    int lineIndex = scrollback.offset + y;
    auto &line = scrollback.lines[lineIndex];

    TSpan<TScreenCell> rowCells(&surface.at(y, 0), surface.size.x);
    VTermScreenCell defCell = getDefaultCell();
    for (int x = 0; x < (int) rowCells.size(); ++x)
    {
        if (x < line.cols)
            convCell(rowCells, x, line.cells.get()[x]);
        else
            convCell(rowCells, x, defCell);
    }
    surface.addDamageAtRow(y, 0, rowCells.size());
}

inline VTermScreenCell VTermEmulator::getDefaultCell() const
{
    VTermScreenCell cell {};
    cell.width = 1;
    vterm_state_get_default_colors(vtState, &cell.fg, &cell.bg);
    return cell;
}

void VTermEmulator::Scrollback::pushLine(int cols, const VTermScreenCell *src, bool continuation)
{
    bool isFollowingTerminal = (offset == numLines());

    if (numLines() >= maxSize)
    {
        lines.pop_front();
        if (isFollowingTerminal)
            offset = max(offset - 1, 0);
    }

    auto *line = new VTermScreenCell[cols];
    memcpy(line, src, sizeof(VTermScreenCell)*cols);
    lines.push_back({std::unique_ptr<const VTermScreenCell[]>(line), cols, continuation});

    if (isFollowingTerminal)
        offset = numLines();
}

void VTermEmulator::Scrollback::incrementOffset(int count)
// count < 0: show older lines
// count > 0: show newer lines
{
    if (count < 0)
        offset = max(offset + count, 0);
    else
        offset = min(offset + count, numLines());
}

void VTermEmulator::Scrollback::followTerminal()
{
    offset = numLines();
}

int VTermEmulator::Scrollback::getVisibleScrollLines(int terminalHeight) const
{
    int available = max(numLines() - offset, 0);
    return min(available, terminalHeight);
}

int VTermEmulator::Scrollback::numLines() const
{
    return lines.size();
}

} // namespace tvterm
