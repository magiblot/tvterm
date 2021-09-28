#ifndef TVTERM_IO_H
#define TVTERM_IO_H

#include <thread>
#include <vector>
#include <asio/io_context.hpp>
#include <asio/executor_work_guard.hpp>

class IOContext
{
public:

    asio::io_context io;

private:

    std::vector<std::thread> threads;
    asio::executor_work_guard<decltype(io.get_executor())> work;

public:

    IOContext();
    ~IOContext();

    operator asio::io_context& ()
    {
        return io;
    }

    operator const asio::io_context& () const
    {
        return io;
    }

private:

    void run(asio::io_context &io);

};

#endif // TVTERM_IO_H
