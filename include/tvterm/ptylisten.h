#ifndef TVTERM_PTYLISTEN_H
#define TVTERM_PTYLISTEN_H

#include <tvision/internal/platform.h>

struct TVTermAdapter;

struct PTYListener : public FdInputStrategy
{

    TVTermAdapter &vterm;

    PTYListener(TVTermAdapter &vterm, int fd);

    bool getEvent(TEvent &ev) override;

};

inline PTYListener::PTYListener(TVTermAdapter &vterm, int fd) :
    vterm(vterm)
{
    addListener(this, fd);
}

#endif // TVTERM_PTYLISTEN_H
