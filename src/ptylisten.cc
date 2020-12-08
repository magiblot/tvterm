#include <tvterm/ptylisten.h>
#include <tvterm/util.h>
#include <unistd.h>
#include <fcntl.h>
#include <string_view>

PTYListener::PTYListener(int master_fd) :
    master_fd(master_fd)
{
    fd_set_flags(master_fd, O_NONBLOCK);
    addListener(this, master_fd);
}

PTYListener::~PTYListener()
{
    cerr << "PTYListener::~PTYListener: close(master_fd) = " << close(master_fd) << endl;
}

bool PTYListener::getEvent(TEvent &ev)
{
    cerr << "PTYListener::getEvent: read(" << master_fd << ") BEGIN" << endl;
    int rr;
    char buf[256];
    while ((rr = read(master_fd, buf, sizeof(buf))) > 0)
    {
        cerr << std::string_view {buf, (size_t) rr};
    }
    cerr << endl
         << "PTYListener::getEvent: read(" << master_fd << ") END" << endl;
    return false;
}
