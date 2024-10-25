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
               nullptr,
               &TVTermApp::initDeskTop
             )
{
    disableCommands(tileCmds);
    for (ushort cmd : TerminalWindow::appConsts.focusedCmds())
        disableCommand(cmd);
    newTerm();
}

TStatusLine *TVTermApp::initStatusLine(TRect r)
{
    r.b.y = r.a.y + 1;
    TStatusLine *statusLine = new TStatusLine(r,
        *new TStatusDef(hcDragging, hcDragging) +
            *new TStatusItem("~Arrow~ Move", kbNoKey, 0) +
            *new TStatusItem("~Shift-Arrow~ Resize", kbNoKey, 0) +
            *new TStatusItem("~Enter~ Done", kbNoKey, 0) +
            *new TStatusItem("~Esc~ Cancel", kbNoKey, 0) +
        *new TStatusDef(hcInputGrabbed, hcInputGrabbed) +
            *new TStatusItem("~Alt-End~ Release Input", kbAltEnd, cmReleaseInput) +
        *new TStatusDef(hcMenu, hcMenu) +
            *new TStatusItem("~Esc~ Close Menu", kbNoKey, 0) +
        *new TStatusDef(0, 0xFFFF) +
            *new TStatusItem("~Ctrl-B~ Open Menu" , kbCtrlB, cmMenu)
            );
    statusLine->growMode = gfGrowHiX;
    return statusLine;
}

TDeskTop *TVTermApp::initDeskTop(TRect r)
{
    r.a.y += 1;
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
                case cmMenu: openMenu(); break;
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
    size_t count = 0;
    message(this, evBroadcast, cmGetOpenTerms, &count);
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

void TVTermApp::openMenu()
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
        newLine() +
        ( *new TSubMenu("~M~ore...", kbNoKey, hcMenu) +
            *new TMenuItem("~C~hange working dir...", cmChangeDir, kbNoKey) +
            newLine() +
            *new TMenuItem("Maximi~z~e/Restore", cmZoom, kbNoKey) +
            *new TMenuItem("C~a~scade", cmCascade, kbNoKey) +
            *new TMenuItem("~G~rab Input", cmGrabInput, kbNoKey)
        ) +
        *new TMenuItem("Suspend", cmDosShell, 'U', hcNoContext, "~U~") +
        *new TMenuItem("Exit", cmQuit, 'Q', hcNoContext, "~Q~");

    TMenu *menu = new TMenu(menuItems);
    TMenuPopup *menuPopup = new CenteredMenuPopup(menu);
    menuPopup->helpCtx = hcMenu;

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
