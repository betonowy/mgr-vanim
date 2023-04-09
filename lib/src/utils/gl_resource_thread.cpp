#include "gl_resource_thread.hpp"

#include <SDL2/SDL_video.h>

#include <gl/message_callback.hpp>

#include <iostream>

namespace utils
{
gl_resource_thread::gl_resource_thread(SDL_GLContext context, SDL_Window *window)
{
    _worker = std::thread([this, context, window]() {
        task task;

        if (SDL_GL_MakeCurrent(window, context))
        {
            throw std::runtime_error(SDL_GetError());
        }

#ifndef NDEBUG
        glDebugMessageCallback(gl::message_callback, "Resource GL Context");
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

        std::cout << "OpenGL resource thread started.\n";

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

gl_resource_thread::~gl_resource_thread()
{
    {
        std::lock_guard lock(_queue_mtx);
        _stopping = true;
    }
    _cvar.notify_all();

    _worker.join();
}
} // namespace utils
