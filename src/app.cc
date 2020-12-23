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
#include <tvision/tv.h>

#include <tvterm/app.h>
#include <tvterm/cmds.h>
#include <tvterm/desk.h>
#include <tvterm/util.h>
#include <tvterm/vtermwnd.h>
#include <tvterm/vtermview.h>

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

void tvterm_main(int, char**)
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
    disableCommands(TVTermWindow::focusedCmds);
    newTerm();
}

TMenuBar *TVTermApp::initMenuBar(TRect r)
{
    r.b.y = r.a.y+1;
    return new TMenuBar( r,
        *new TSubMenu( "~F~ile", kbAltF, hcNoContext ) +
            *new TMenuItem( "~N~ew", cmNewTerm, kbCtrlN, hcNoContext, "Ctrl-N" ) +
            newLine() +
            *new TMenuItem( "~C~hange dir...", cmChangeDir, kbNoKey ) +
            *new TMenuItem( "S~u~spend", cmDosShell, kbNoKey, hcNoContext ) +
            *new TMenuItem( "E~x~it", cmQuit, kbAltX, hcNoContext, "Alt-X" ) +
        *new TSubMenu( "~W~indows", kbAltW ) +
            *new TMenuItem( "~S~ize/move",cmResize, kbCtrlF5, hcNoContext, "Ctrl-F5" ) +
            *new TMenuItem( "~Z~oom", cmZoom, kbF5, hcNoContext, "F5" ) +
            *new TMenuItem( "~T~ile (Columns First)", cmTileCols, kbNoKey ) +
            *new TMenuItem( "Tile (~R~ows First)", cmTileRows, kbNoKey ) +
            *new TMenuItem( "C~a~scade", cmCascade, kbNoKey ) +
            *new TMenuItem( "~N~ext", cmNext, kbF6, hcNoContext, "F6" ) +
            *new TMenuItem( "~P~revious", cmPrev, kbShiftF6, hcNoContext, "Shift-F6" ) +
            *new TMenuItem( "~C~lose", cmClose, kbAltF3, hcNoContext, "Alt+F3" )
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
            *new TStatusItem( "~Ctrl-N~ New", kbNoKey, cmNewTerm ) +
            *new TStatusItem( "~F6~ Next", kbNoKey, cmNext ) +
            *new TStatusItem( "~Alt-PgUp~ Grab Input", kbAltPgUp, cmGrabInput ) +
            *new TStatusItem( "~F12~ Menu" , kbF12, cmMenu )
            );
}

TDeskTop *TVTermApp::initDeskTop( TRect r )
{
    r.grow(0, -1);
    return new TVTermDesk(r);
}

void TVTermApp::getEvent(TEvent &ev)
{
    TApplication::getEvent(ev);
    if (ev.what == evCommand && ev.message.command == cmVTermReadable)
    {
        ((TVTermAdapter *) ev.message.infoPtr)->read();
        clearEvent(ev);
    }
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
}

// Command handlers

void TVTermApp::newTerm()
{
    TRect r = deskTop->getExtent();
    TVTermWindow *vt = new TVTermWindow(r);
    vt = (TVTermWindow *) validView(vt);
    if (vt)
        deskTop->insert(vt);
}

void TVTermApp::changeDir()
{
    execDialog(new TChDirDialog(cdNormal, 0));
}
