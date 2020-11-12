#include <tvterm/app.h>

TVTermApp* TVTermApp::app;

TVTermApp::TVTermApp() :
    TProgInit( &TVTermApp::initStatusLine,
               &TVTermApp::initMenuBar,
               &TVTermApp::initDeskTop
             )
{
}
