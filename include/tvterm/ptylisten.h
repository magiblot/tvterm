#ifndef TVTERM_PTYLISTEN_H
#define TVTERM_PTYLISTEN_H

#define Uses_TArc
#include <tvision/tv.h>
#include <tvision/internal/mutex.h>

#include <tvterm/io.h>

struct TVTermAdapter;

using stream_descriptor = asio::posix::stream_descriptor;

struct PTYListener
{

    TVTermAdapter &vterm;
    stream_descriptor descriptor;
    TArc<TMutex<bool>> mAlive;
    enum { maxEmpty = 5 };
    int emptyCount;

    PTYListener(TVTermAdapter &vterm, asio::io_context &io, int fd);
    ~PTYListener();

    void asyncWait();
    bool streamNotEmpty();
    void onReady(const asio::error_code &error);

};


#endif // TVTERM_PTYLISTEN_H
