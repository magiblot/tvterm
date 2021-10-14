#ifndef TVTERM_THREADPOOL_H
#define TVTERM_THREADPOOL_H

#include <mutex>
#include <thread>
#include <utility>
#include <condition_variable>

namespace tvterm
{

class ThreadPool
{
    std::mutex m;
    std::condition_variable cv;
    size_t threadCount {0};

    struct Deleter
    {
        void operator()(ThreadPool *p) noexcept
        {
            if (p)
            {
                {
                    std::lock_guard<std::mutex> lk {p->m};
                    --p->threadCount;
                }
                p->cv.notify_one();
            }
        }
    };

    using Guard = std::unique_ptr<ThreadPool, Deleter>;

public:

    ~ThreadPool()
    {
        std::unique_lock<std::mutex> lk {m};
        while (threadCount != 0)
            cv.wait(lk);
    }

    template <class Func>
    void run(Func &&func) noexcept
    {
        {
            std::lock_guard<std::mutex> lk {m};
            ++threadCount;
        }
        std::thread([g = Guard {this}, func = std::move(func)] () noexcept {
            func();
        }).detach();
    }
};

} // namespace tvterm

#endif // TVTERM_THREADPOOL_H
