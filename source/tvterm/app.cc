#define Uses_TMenuBar
#define Uses_TSubMenu
#define Uses_TMenu
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
#include <tvterm/termctrl.h>
#include <tvterm/vtermemu.h>

#include <stdlib.h>
#include <signal.h>

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
    app.run();
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
    r.b.y = r.a.y + 1;
    return new TMenuBar(r,
        *new TSubMenu("File", kbNoKey) +
            *new TMenuItem("~N~ew Term", cmNewTerm, kbNoKey) +
            newLine() +
            *new TMenuItem("~C~hange dir...", cmChangeDir, kbNoKey) +
            *new TMenuItem("S~u~spend", cmDosShell, kbNoKey) +
            *new TMenuItem("E~x~it", cmQuit, kbNoKey) +
        *new TSubMenu("Windows", kbNoKey) +
            *new TMenuItem("Re~s~ize/Move", cmResize, kbNoKey) +
            *new TMenuItem("Maximi~z~e/Restore", cmZoom, kbNoKey) +
            *new TMenuItem("~T~ile (Columns First)", cmTileCols, kbNoKey) +
            *new TMenuItem("Tile (~R~ows First)", cmTileRows, kbNoKey) +
            *new TMenuItem("C~a~scade", cmCascade, kbNoKey) +
            *new TMenuItem("~N~ext", cmNext, kbNoKey) +
            *new TMenuItem("~P~revious", cmPrev, kbNoKey) +
            *new TMenuItem("~C~lose", cmClose, kbNoKey)
            );
}

TStatusLine *TVTermApp::initStatusLine(TRect r)
{
    r.a.y = r.b.y - 1;
    return new TStatusLine(r,
        *new TStatusDef(hcDragging, hcDragging) +
            *new TStatusItem("~Arrow~ Move", kbNoKey, 0) +
            *new TStatusItem("~Shift-Arrow~ Resize", kbNoKey, 0) +
            *new TStatusItem("~Enter~ Done", kbNoKey, 0) +
            *new TStatusItem("~Esc~ Cancel", kbNoKey, 0) +
        *new TStatusDef(hcInputGrabbed, hcInputGrabbed) +
            *new TStatusItem("~Alt-PgDn~ Release Input", kbAltPgDn, cmReleaseInput) +
        *new TStatusDef(hcQuickMenu, hcQuickMenu) +
            *new TStatusItem("~Esc~ Hide Menu", kbEsc, 0) +
        *new TStatusDef(0, 0xFFFF) +
            *new TStatusItem("~Ctrl-B~ Quick Menu" , kbCtrlB, cmQuickMenu)
            );
}

TDeskTop *TVTermApp::initDeskTop(TRect r)
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
                case cmQuickMenu: showQuickMenu(); break;
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
    message(this, evBroadcast, cmCheckTerminalUpdates, nullptr);
}

void TVTermApp::showQuickMenu()
{
    TMenuItem &menuItems =
        *new TMenuItem("New Term", cmNewTerm, 'N', hcNoContext, "~N~") +
        *new TMenuItem("Close Term", cmClose, 'W', hcNoContext, "~W~") +
        newLine() +
        *new TMenuItem("Next Term", cmNext, kbTab, hcNoContext, "~Tab~") +
        *new TMenuItem("Previous Term", cmPrev, kbShiftTab, hcNoContext, "~Shift-Tab~") +
        *new TMenuItem("Tile (Columns First)", cmTileCols, 'V', hcNoContext, "~V~") +
        *new TMenuItem("Tile (Rows First)", cmTileRows, 'H', hcNoContext, "~H~") +
        *new TMenuItem("Resize", cmResize, 'R', hcNoContext, "~R~") +
        *new TMenuItem("Grab Input", cmGrabInput, 'I', hcNoContext, "~I~") +
        newLine() +
        *new TMenuItem("Top Menu", cmMenu, 'M', hcNoContext, "~M~") +
        *new TMenuItem("Suspend", cmDosShell, 'U', hcNoContext, "~U~") +
        *new TMenuItem("Exit", cmQuit, 'Q', hcNoContext, "~Q~");

    TMenu *menu = new TMenu(menuItems);
    TMenuPopup *menuPopup = new CenteredMenuPopup(menu);
    menuPopup->helpCtx = hcQuickMenu;

    if (ushort cmd = execView(menuPopup))
    {
        TEvent event = {};
        event.what = evCommand;
        event.message.command = cmd;
        putEvent(event);
    }

    TObject::destroy(menuPopup);
    delete menu;
}

static void onTermError(const char *reason)
{
    messageBox(mfError | mfOKButton, "Cannot create terminal: %s.", reason);
};

void TVTermApp::newTerm()
{
    using namespace tvterm;
    TRect r = deskTop->getExtent();
    VTermEmulatorFactory factory;
    auto *termCtrl = TerminalController::create( TerminalWindow::viewSize(r),
                                                 factory, onTermError );
    if (termCtrl)
        insertWindow(new TerminalWindow(r, *termCtrl));
}

void TVTermApp::changeDir()
{
    execDialog(new TChDirDialog(cdNormal, 0));
}
