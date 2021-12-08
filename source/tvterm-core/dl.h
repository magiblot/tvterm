#ifndef TVTERM_DL_H
#define TVTERM_DL_H

#include <string.h>
#include <dlfcn.h>
#include <memory>

namespace tvterm
{

class DynLib
{
    void *handle;

    DynLib(void *aHandle) noexcept :
        handle(aHandle)
    {
    }

public:

    template <size_t lname>
    static DynLib *open(const char (&name)[lname]) noexcept
    {
        char soname[lname + 7];
        memcpy(&soname[0], name, lname - 1);
#ifdef __APPLE__
        strcpy(&soname[lname - 1], ".dylib");
#else
        strcpy(&soname[lname - 1], ".so");
#endif
        if (auto *handle = ::dlopen(&soname[0], RTLD_LAZY))
            return new DynLib(handle);
        return nullptr;
    }

    ~DynLib()
    {
        ::dlclose(handle);
    }

    void *getProc(const char *name) noexcept
    {
        return ::dlsym(handle, name);
    }

    static const char *getLastError() noexcept
    {
        return ::dlerror();
    }
};

} // namespace tvterm

#endif // TVTERM_DL_H
