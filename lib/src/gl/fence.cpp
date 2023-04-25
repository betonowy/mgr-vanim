#include "fence.hpp"

#include <utility>

namespace gl
{
fence::fence()
{
    _object = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

fence::~fence()
{
    if (_object)
    {
        glDeleteSync(_object);
    }
}

fence::fence(fence &&other) : _object(std::exchange(other._object, nullptr))
{
}

fence &fence::operator=(fence &&other)
{
    _object = std::exchange(other._object, nullptr);
    return *this;
}

void fence::sync()
{
    if (_object)
    {
        glDeleteSync(_object);
    }

    _object = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

GLenum fence::client_wait(GLuint64 nanos, bool flush)
{
    return glClientWaitSync(_object, flush ? GL_SYNC_FLUSH_COMMANDS_BIT : 0, nanos);
}

void fence::server_wait()
{
    glWaitSync(_object, 0, GL_TIMEOUT_IGNORED);
}
} // namespace gl
