#include "thread_pool.hpp"

#include <iostream>

namespace utils
{
thread_pool::thread_pool(size_t count)
{
    if (count == 0)
    {
        count = 1;
    }

    for (size_t i = 0; i < count; ++i)
    {
        _workers.emplace_back([this]() {
            task task;

            while (true)
            {
                {
                    std::unique_lock lock(_queue_mtx);

                    _cvar.wait(lock, [this] { return _stopping || !_task_queue.empty(); });

                    if (_stopping && _task_queue.empty())
                    {
                        break;
                    }

                    task = std::move(_task_queue.front());
                    _task_queue.pop();
                }

                task();
            }
        });
    }

    std::cout << __FUNCTION__ << ": " << count << " workers started.\n";
}

thread_pool::~thread_pool()
{
    {
        std::lock_guard lock(_queue_mtx);
        _stopping = true;
    }
    _cvar.notify_all();

    for (auto &worker : _workers)
    {
        worker.join();
    }
}
} // namespace utils
