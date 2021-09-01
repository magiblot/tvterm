#ifndef TVTERM_PTYLISTEN_H
#define TVTERM_PTYLISTEN_H

#define Uses_TArc
#include <tvision/tv.h>
#include <tvision/internal/mutex.h>

#include <tvterm/io.h>
#include <chrono>

struct TVTermAdapter;

struct PTYListener
{

    using clock = std::chrono::steady_clock;
    using time_point = clock::time_point;

    enum { bufSize = 4096 };
    enum { maxEmpty = 5 };
    static constexpr auto maxReadTime = std::chrono::milliseconds(20);
    static constexpr auto waitForFurtherDataDelay = std::chrono::milliseconds(5);

    TVTermAdapter &vterm;
    asio::posix::stream_descriptor descriptor;
    asio::basic_waitable_timer<clock> timer;
    TArc<TMutex<bool>> mAlive;
    int emptyCount;

    static thread_local char localBuf alignas(4096) [bufSize];

    PTYListener(TVTermAdapter &vterm, asio::io_context &io, int fd);

    void start();
    void stop();

private:

    bool streamNotEmpty();
    void asyncWait();
    template <class Func>
    void asyncReadUntil(time_point timeout, Func &&);
    void waitHandler(const asio::error_code &error);
    void readInput(TSpan<const char> buf, time_point limit);

};

#endif // TVTERM_PTYLISTEN_H
