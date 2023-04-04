#pragma once

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace utils
{
class thread_pool
{
public:
    using task = std::function<void()>;

    thread_pool(size_t count);
    ~thread_pool();

    template <class T> auto enqueue(T task) -> std::future<decltype(task())>
    {
        auto wrapper = std::make_shared<std::packaged_task<decltype(task())()>>(std::move(task));
        auto future = wrapper->get_future();

        {
            std::lock_guard<std::mutex> lock(_queue_mtx);
            _task_queue.emplace([wrapper = std::move(wrapper)] { (*wrapper)(); });
        }

        _cvar.notify_one();
        return future;
    }

private:
    std::mutex _queue_mtx;
    std::condition_variable _cvar;
    bool _stopping = false;

    std::vector<std::thread> _workers;
    std::queue<task> _task_queue;
};
}; // namespace utils
