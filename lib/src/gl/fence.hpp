#pragma once

#include <GL/gl3w.h>

#include <utility>

namespace gl
{
class fence
{
public:
    fence() = default;
    ~fence()
    {
        if (_object)
        {
            glDeleteSync(_object);
        }
    }

    fence(const fence &) = delete;
    fence &operator=(const fence &) = delete;

    fence(fence &&other) : _object(std::exchange(other._object, nullptr))
    {
    }

    fence &operator=(fence &&other)
    {
        _object = std::exchange(other._object, nullptr);
        return *this;
    }

    void sync()
    {
        if (_object)
        {
            glDeleteSync(_object);
        }

        _object = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }

    GLenum client_wait(bool flush)
    {
        if (_object)
        {
            return glClientWaitSync(_object, flush ? GL_SYNC_FLUSH_COMMANDS_BIT : 0, 1000000000);
        }

        return GL_ALREADY_SIGNALED;
    }

    void server_wait()
    {
        if (_object)
        {
            glWaitSync(_object, 0, GL_TIMEOUT_IGNORED);
        }
    }

private:
    GLsync _object = nullptr;
};
} // namespace gl
