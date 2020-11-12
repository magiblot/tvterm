#include <tvterm/app.h>

int main()
{
    TVTermApp app;
    TVTermApp::app = &app;
    app.run();
    TVTermApp::app = nullptr;
    app.shutDown();
}
