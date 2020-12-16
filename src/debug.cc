#include <tvterm/debug.h>

#include <cstdlib>

DebugCout DebugCout::instance;

DebugCout::DebugCout() :
    nbuf(),
    nout(&nbuf)
{
    const char *env = getenv("TVTERM_DEBUG");
    enabled = env && *env;
}
