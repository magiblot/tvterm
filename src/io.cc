#include <tvterm/io.h>
#include <functional>
#include <thread>
#include <chrono>

IOContext::IOContext() :
    maxThreads(std::thread::hardware_concurrency()),
    work(io.get_executor())
{
    threads.reserve(maxThreads);
}

void IOContext::makeRoom(size_t count)
{
    size_t newCount = count < maxThreads ? count : maxThreads;
    for (size_t i = threads.size(); i < newCount; ++i)
        threads.emplace_back(&IOContext::run, this);
}

IOContext::~IOContext()
{
    work.reset();
    io.stop();
    for (auto &thread : threads)
        thread.join();
}

void IOContext::run()
{
    io.run();
}
