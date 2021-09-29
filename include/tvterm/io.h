#ifndef TVTERM_IO_H
#define TVTERM_IO_H

#include <thread>
#include <vector>
#include <asio/io_context.hpp>
#include <asio/executor_work_guard.hpp>

class IOContext
{
    std::vector<std::thread> threads;
    const size_t maxThreads;
    asio::io_context io;
    asio::executor_work_guard<decltype(io.get_executor())> work;

    void run();

public:

    IOContext();
    ~IOContext();

    void makeRoom(size_t strandCount);

    asio::io_context& getContext()
    {
        return io;
    }
};

#endif // TVTERM_IO_H
