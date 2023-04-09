#pragma once

#include "object.hpp"

#include <glm/vec2.hpp>

#include <memory>
#include <vector>

typedef struct SDL_Window SDL_Window;
typedef void *SDL_GLContext;

namespace utils
{
class thread_pool;
class gl_resource_thread;
} // namespace utils

namespace scene
{
class scene;
enum class signal_e;

class object_context
{
    friend scene;

    object_context(SDL_GLContext, SDL_Window *);
    ~object_context();

    object_context(object_context &&) = delete;
    object_context &operator=(object_context &&) = delete;

public:
    void add_object(std::shared_ptr<object>);

    template <typename T> std::shared_ptr<T> share_object(std::shared_ptr<T> obj)
    {
        add_object(obj);
        return obj;
    }

    glm::ivec2 window_size()
    {
        return _window_size;
    }

    utils::thread_pool &generic_thread_pool()
    {
        return *_cpu_thread_pool;
    }

    const std::shared_ptr<utils::thread_pool> &generic_thread_pool_sptr()
    {
        return _cpu_thread_pool;
    }

    utils::gl_resource_thread &gl_thread()
    {
        return *_gl_thread;
    }

    const std::shared_ptr<utils::thread_pool> &gl_thread_sptr()
    {
        return _cpu_thread_pool;
    }

    void broadcast_signal(signal_e);

    std::string resource_directory() const;

    template <typename T> std::vector<std::shared_ptr<T>> find_objects()
    {
        std::vector<std::shared_ptr<T>> found_objects;

        for (const auto &object : _objects)
        {
            if (dynamic_cast<T *>(object.get()))
            {
                found_objects.push_back(std::dynamic_pointer_cast<T>(object));
            }
        }

        for (const auto &object : _objects_to_init)
        {
            if (dynamic_cast<T *>(object.get()))
            {
                found_objects.push_back(std::dynamic_pointer_cast<T>(object));
            }
        }

        return found_objects;
    }

private:
    std::shared_ptr<utils::thread_pool> _cpu_thread_pool;
    std::shared_ptr<utils::gl_resource_thread> _gl_thread;

    std::vector<std::shared_ptr<object>> _objects_to_add;
    std::vector<std::shared_ptr<object>> _objects_to_init;
    std::vector<std::shared_ptr<object>> _objects;
    std::vector<std::shared_ptr<object>> _objects_to_delete;
    std::vector<signal_e> _pending_signals;

    std::string base_directory;

    glm::ivec2 _window_size;
};
} // namespace scene
