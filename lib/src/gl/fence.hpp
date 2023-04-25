#pragma once

#include <GL/gl3w.h>

namespace gl
{
class fence
{
public:
    fence();
    ~fence();

    fence(const fence &) = delete;
    fence &operator=(const fence &) = delete;

    fence(fence &&);
    fence &operator=(fence &&);

    void sync();

    GLenum client_wait(GLuint64 nanos, bool flush);

    void server_wait();

private:
    GLsync _object;
};
} // namespace gl
