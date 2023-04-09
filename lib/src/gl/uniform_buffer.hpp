#pragma once

#include <GL/gl3w.h>

#include <cstddef>

namespace gl
{
class uniform_buffer
{
public:
    uniform_buffer();
    ~uniform_buffer();

    operator GLuint() const
    {
        return _id;
    }

    void buffer_data(const void *, size_t, GLenum usage);

    void set_buffer_base(int);

    GLuint get_buffer_base() const;

private:
    GLuint _id = 0;
    size_t _tracked_size = 0;
    GLuint _base_index = 0;
};
} // namespace gl
