#include "object_context.hpp"

#include <utils/gl_resource_thread.hpp>
#include <utils/thread_pool.hpp>

namespace scene
{
object_context::object_context(SDL_GLContext gl_context, SDL_Window *window)
    : _cpu_thread_pool(std::make_shared<utils::thread_pool>(/*std::thread::hardware_concurrency() / 2*/ 1)), _gl_thread(std::make_shared<utils::gl_resource_thread>(gl_context, window))
{
}

object_context::~object_context() = default;

void object_context::add_object(std::shared_ptr<object> obj)
{
    _objects_to_add.push_back(std::move(obj));
}

void object_context::broadcast_signal(signal_e new_signal)
{
    for (const auto &signal : _pending_signals)
    {
        if (signal == new_signal)
        {
            return;
        }
    }

    _pending_signals.push_back(new_signal);
}
} // namespace scene
