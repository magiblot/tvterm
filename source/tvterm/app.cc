#define Uses_TMenuBar
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TStatusLine
#define Uses_TStatusItem
#define Uses_TStatusDef
#define Uses_TKeys
#define Uses_TEvent
#define Uses_TChDirDialog
#define Uses_TDeskTop
#define Uses_MsgBox
#include <tvision/tv.h>

#include "app.h"
#include "cmds.h"
#include "desk.h"
#include "wnd.h"
#include "apputil.h"
#include <tvterm/termactiv.h>
#include <tvterm/vtermadapt.h>

#include <stdlib.h>
#include <signal.h>

TVTermApp* TVTermApp::app;
TCommandSet TVTermApp::tileCmds = []()
{
    TCommandSet ts;
    ts += cmTile;
    ts += cmTileCols;
    ts += cmTileRows;
    ts += cmCascade;
    return ts;
}();

int main(int, char**)
{
    TVTermApp app;
    TVTermApp::app = &app;
    app.run();
    TVTermApp::app = nullptr;
    app.shutDown();
}

TVTermApp::TVTermApp() :
    TProgInit( &TVTermApp::initStatusLine,
               &TVTermApp::initMenuBar,
               &TVTermApp::initDeskTop
             )
{
    disableCommands(tileCmds);
    for (ushort cmd : TerminalWindow::appConsts.focusedCmds())
        disableCommand(cmd);
    newTerm();
}

TMenuBar *TVTermApp::initMenuBar(TRect r)
{
    r.b.y = r.a.y+1;
    return new TMenuBar( r,
        *new TSubMenu( "~F~ile", kbAltF, hcNoContext ) +
            // The rational for using Alt+_ shortctus for window management is that
            // most TUI apps are not sofisticated enough to need the Alt key
            // for shortcuts (usually only F-keys and sometimes Ctrl are used).
            // This frees us to use Alt + <intuitive letter> with only a minimal chance
            // of it clashing with an app.

            *new TMenuItem( "~N~ew Window", cmNewTerm, kbAltN, hcNoContext, "Alt-N" ) +
            newLine() +
            *new TMenuItem( "~C~hange dir...", cmChangeDir, kbNoKey ) +
            *new TMenuItem( "S~u~spend", cmDosShell, kbNoKey, hcNoContext ) +
            *new TMenuItem( "E~x~it", cmQuit, kbAltQ, hcNoContext, "Alt-Q" ) +
        *new TSubMenu( "~W~indows", kbAltW ) +
            *new TMenuItem( "~M~ove/Resize",cmResize, kbAltM, hcNoContext, "Alt-M" ) +
            *new TMenuItem( "~Z~oom", cmZoom, kbAltZ, hcNoContext, "Alt-Z" ) +
            *new TMenuItem( "~T~ile (Columns First)", cmTileCols, kbAltT, hcNoContext, "Alt-T" ) +
            *new TMenuItem( "Tile (~R~ows First)", cmTileRows, kbNoKey ) +
            *new TMenuItem( "C~a~scade", cmCascade, kbNoKey ) +
            *new TMenuItem( "~N~ext", cmNext, kbAltEqual, hcNoContext, "Alt-+" ) +
            *new TMenuItem( "~P~revious", cmPrev, kbAltMinus, hcNoContext, "Alt--" ) +
            *new TMenuItem( "~C~lose", cmClose, kbAltX, hcNoContext, "Alt-X" /*TODO: Also Alt-W (what ppl are used to)*/ )
    );
}

TStatusLine *TVTermApp::initStatusLine( TRect r )
{
    r.a.y = r.b.y-1;
    return new TStatusLine( r,
        *new TStatusDef( hcDragging, hcDragging ) +
            *new TStatusItem( "~Arrow~ Move", kbNoKey, cmValid ) +
            *new TStatusItem( "~Shift-Arrow~ Resize", kbNoKey, cmValid ) +
            *new TStatusItem( "~Enter~ Done", kbNoKey, cmValid ) +
            *new TStatusItem( "~Esc~ Abort", kbNoKey, cmValid ) +
        *new TStatusDef( hcInputGrabbed, hcInputGrabbed ) +
            *new TStatusItem( "~Alt-PgDn~ Release Input", kbAltPgDn, cmReleaseInput ) +
        *new TStatusDef( 0, 0xFFFF ) +
            *new TStatusItem( "~Alt-Q~ Quit", kbNoKey, cmNewTerm ) +
            *new TStatusItem( "~F12~ Menu" , kbF12, cmMenu ) +
            *new TStatusItem( "~Alt-PgUp~ Grab Input", kbAltPgUp, cmGrabInput )
            );
}

TDeskTop *TVTermApp::initDeskTop( TRect r )
{
    r.grow(0, -1);
    return new TVTermDesk(r);
}

void TVTermApp::handleEvent(TEvent &event)
{
    TApplication::handleEvent(event);
    bool handled = true;
    switch (event.what)
    {
        case evCommand:
            switch (event.message.command)
            {
                case cmNewTerm: newTerm(); break;
                case cmChangeDir: changeDir(); break;
                case cmTileCols: getDeskTop()->tileVertical(getTileRect()); break;
                case cmTileRows: getDeskTop()->tileHorizontal(getTileRect()); break;
                default:
                    handled = false;
                    break;
            }
            break;
        default:
            handled = false;
            break;
    }
    if (handled)
        clearEvent(event);
}

size_t TVTermApp::getOpenTermCount()
{
    auto getOpenTerms = [] (TView *p, void *infoPtr)
    {
        message(p, evBroadcast, cmGetOpenTerms, infoPtr);
    };
    size_t count = 0;
    deskTop->forEach(getOpenTerms, &count);
    return count;
}

Boolean TVTermApp::valid(ushort command)
{
    if (command == cmQuit)
    {
        if (size_t count = getOpenTermCount())
        {
            auto *format = (count == 1)
                ? "There is %zu open terminal window.\nDo you want to quit anyway?"
                : "There are %zu open terminal windows.\nDo you want to quit anyway?";
            return messageBox(
                mfConfirmation | mfYesButton | mfNoButton,
                format, count
            ) == cmYes;
        }
        return True;
    }
    return TApplication::valid(command);
}

void TVTermApp::idle()
{
    TApplication::idle();
    {
        // Enable or disable the cmTile and cmCascade commands.
        auto isTileable =
            [] (TView *p, void *) -> Boolean { return p->options & ofTileable; };
        if (deskTop->firstThat(isTileable, nullptr))
            enableCommands(tileCmds);
        else
            disableCommands(tileCmds);
    }
    message(this, evBroadcast, cmIdle, nullptr);
}

static void onTermError(const char *reason)
{
    messageBox(mfError | mfOKButton, "Cannot create terminal: %s.", reason);
};

void setCenter(TRect& rect, const TPoint pt)
{
    int w = (rect.b.x - rect.a.x);
    int h = (rect.b.y - rect.a.y);

    rect.a.x = pt.x - w/2;
    rect.a.y = pt.y - h/2;
    rect.b.x = pt.x - w/2 + w;  // Keeps odd sizes
    rect.b.y = pt.y - h/2 + h;
}

TPoint getCenter(const TRect& rect)
{
    TPoint c = {
        (rect.a.x + rect.b.x) / 2,
        (rect.a.y + rect.b.y) / 2
    };
    return c;
}

TRect TVTermApp::newWindowCoords()
{
    size_t count = getOpenTermCount();
    TRect dtop_bounds = deskTop->getExtent();

    if (count == 0)
    {
        // If there aren't any windows, fill the whole desktop
        return deskTop->getExtent();
    }
    else if (count == 1)
    {
        // If there is a single window that is maximized, split the desktop equally.
        TView *existing = deskTop->first();

        if (existing->getBounds() == dtop_bounds)
        {
            TRect left = dtop_bounds;
            left.b.x /= 2;      // Make it cover only half the screen
            existing->changeBounds(left);
            
            TRect right = dtop_bounds;
            right.a.x = left.b.x;
            return right;
        }
        // Else maximized
        return dtop_bounds;
    }
    else if (count == 2)
    {
        // If there are 2 tiled windows, start making floating ones
        if (deskTop->first()->getBounds().Union(deskTop->first()->nextView()->getBounds()) == dtop_bounds)
        {
            TRect new_bounds = dtop_bounds;
            // Make the window half the screen size
            new_bounds.b.x /= 2;
            new_bounds.b.y = (dtop_bounds.b.y * 2) / 3;
            setCenter(new_bounds, getCenter(dtop_bounds));
            return new_bounds;
        }

        // Otherwise follow standard Many Window behavior
    }

    // Many windows. The goal is to stagger the windows in a way that they don't overlap each other fully.

    // Same size as the most recently created window
    TView *last_window = deskTop->first();  // Apparently TVision prepends new windows
    TRect new_bounds = last_window->getBounds();

    // Find the center of 'gravity' of existing windows.
    signed int avg_x = 0;
    signed int avg_y = 0;   // Average distance of center of the window from the center of the desktop
    int n = 0;

    TPoint dtop_center = getCenter(dtop_bounds);

    for (TView *cur = deskTop->first(); cur != NULL; cur = cur->nextView())
    {
        TPoint cur_center = getCenter(cur->getBounds());

        if (cur_center.x != dtop_center.x && cur_center.y != dtop_center.y)     // Only count windows that aren't centred in any way.
        {
            avg_x += (cur_center.x - dtop_center.x);
            avg_y += (cur_center.y - dtop_center.y);
        }

        // messageBox(0, "(%d,%d) => (%d,%d)", cur_center.x, cur_center.y, dtop_center.x, dtop_center.y);

        n++;
    }

    avg_x /= n;
    avg_y /= n;

    // Place the new window opposite this average point with respect to the center.
    setCenter(new_bounds, {
        dtop_center.x + (-avg_x),
        dtop_center.y + (-avg_y),
    });

    // If it is still fully overlapping another window (the window layout is probably symmetrical), move it by (1,1).
    do {
        bool unique = true;
        for (TView *cur = deskTop->first(); cur != NULL; cur = cur->nextView())
            if (new_bounds.a == cur->getBounds().a)
                unique = false;
        
        if (!unique)
            new_bounds.move(1, 1);
        else
            return new_bounds;
    } while (1);
}

void TVTermApp::newTerm()
{
    using namespace tvterm;
    TRect r = newWindowCoords();
    auto *term = TerminalActivity::create( TerminalWindow::viewSize(r),
                                           VTermAdapter::create,
                                           VTermAdapter::childActions,
                                           onTermError, threadPool );
    if (term)
        insertWindow(new TerminalWindow(r, *term));
}

void TVTermApp::changeDir()
{
    execDialog(new TChDirDialog(cdNormal, 0));
}
