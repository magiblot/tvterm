#include <tvterm/debug.h>

#include <stdlib.h>

namespace debug
{

    void breakable()
    {
    }

    void (* volatile const breakable_ptr)() = &debug::breakable;

} // namespace debug

DebugCout DebugCout::instance;

DebugCout::DebugCout() :
    nbuf(),
    nout(&nbuf)
{
    const char *env = getenv("TVTERM_DEBUG");
    enabled = env && *env;
}

