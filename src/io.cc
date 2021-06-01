#include <tvterm/io.h>
#include <functional>
#include <chrono>

IOContext::IOContext() :
    work(io.get_executor())
{
    size_t n = 1;
    threads.reserve(n);
    for (size_t i = 0; i < n; ++i)
        threads.emplace_back(&IOContext::run, this, std::ref(io));
}

IOContext::~IOContext()
{
    work.reset();
    io.stop();
    for (auto &thread : threads)
        thread.join();
}

void IOContext::run(asio::io_context &io)
{
    io.run();
}