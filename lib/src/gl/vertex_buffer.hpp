#pragma once

#include <GL/gl3w.h>

#include <cstddef>

namespace gl
{
class vertex_buffer
{
public:
    vertex_buffer();
    ~vertex_buffer();

    vertex_buffer(const vertex_buffer &) = delete;
    vertex_buffer &operator=(const vertex_buffer &) = delete;
    vertex_buffer(vertex_buffer &&) = delete;
    vertex_buffer &operator=(vertex_buffer &&) = delete;

    void buffer_data(void *, size_t, GLenum usage);

    operator GLuint() const
    {
        return _id;
    }

private:
    GLuint _id = 0;
    size_t tracked_size = 0;
};
} // namespace gl
