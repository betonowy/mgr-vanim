#pragma once

#include "object.hpp"

#include <glm/vec2.hpp>

#include <memory>
#include <vector>

namespace utils
{
class thread_pool;
}

namespace scene
{
class scene;
enum class signal_e;

class object_context
{
    friend scene;

    object_context();
    ~object_context();

    object_context(object_context &&) = delete;
    object_context &operator=(object_context &&) = delete;

public:
    void add_object(std::shared_ptr<object>);
    std::shared_ptr<object> share_object(std::shared_ptr<object>);

    glm::ivec2 window_size() { return _window_size; }

    utils::thread_pool &generic_thread_pool() { return *_cpu_thread_pool; }
    const std::shared_ptr<utils::thread_pool> &generic_thread_pool_sptr() { return _cpu_thread_pool; }

    void broadcast_signal(signal_e);

private:
    std::shared_ptr<utils::thread_pool> _cpu_thread_pool;

    std::vector<std::shared_ptr<object>> _objects_to_add;
    std::vector<std::shared_ptr<object>> _objects_to_init;
    std::vector<std::shared_ptr<object>> _objects;
    std::vector<std::shared_ptr<object>> _objects_to_delete;
    std::vector<signal_e> _pending_signals;

    glm::ivec2 _window_size;
};
} // namespace scene
