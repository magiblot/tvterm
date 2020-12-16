#ifndef TVTERM_PTYLISTEN_H
#define TVTERM_PTYLISTEN_H

#include <tvision/internal/platform.h>

struct TVTermAdapter;

struct PTYListener : public FdInputStrategy
{

    TVTermAdapter &vterm;

    PTYListener(TVTermAdapter &vterm);
    void listen(int fd);
    void disconnect();

    bool getEvent(TEvent &ev) override;

};

inline PTYListener::PTYListener(TVTermAdapter &vterm) :
    vterm(vterm)
{
}

inline void PTYListener::listen(int fd)
{
    addListener(this, fd);
}

inline void PTYListener::disconnect()
{
    deleteListener(this);
}

#endif // TVTERM_PTYLISTEN_H
