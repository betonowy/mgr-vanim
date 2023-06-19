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
            while (true)
            {
                {
                    task task;

                    {
                        std::unique_lock lock(_queue_mtx);

                        _cvar_queue.wait(lock, [this] { return _stopping || !_task_queue.empty(); });

                        if (_stopping && _task_queue.empty())
                        {
                            break;
                        }

                        task = std::move(_task_queue.front());
                        _task_queue.pop();
                    }

                    task();
                    --active_tasks;
                }

                _cvar_active_tasks.notify_all();
            }
        });
    }

    std::cout << __func__ << ": " << count << " workers started.\n";
}

thread_pool::~thread_pool()
{
    {
        std::lock_guard lock(_queue_mtx);
        _stopping = true;
    }
    _cvar_queue.notify_all();

    for (auto &worker : _workers)
    {
        worker.join();
    }
}

void thread_pool::work_together()
{
    while (true)
    {
        {
            task task;

            {
                std::lock_guard lock(_queue_mtx);

                if (_task_queue.empty())
                {
                    break;
                }

                task = std::move(_task_queue.front());
                _task_queue.pop();
            }

            task();
            --active_tasks;
        }
        
        _cvar_active_tasks.notify_all();
    }
}

void thread_pool::finish()
{
    work_together();

    std::unique_lock lock(_queue_mtx);

    _cvar_active_tasks.wait(lock, [this] { return active_tasks == 0; });
}
} // namespace utils
