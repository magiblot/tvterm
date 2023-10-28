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

void TVTermApp::newTerm()
{
    using namespace tvterm;
    TRect r = deskTop->getExtent();
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
