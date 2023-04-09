#pragma once

#include <GL/gl3w.h>

#include <cstddef>

namespace gl
{
class shader_storage
{
public:
    shader_storage();
    ~shader_storage();

    operator GLuint() const
    {
        return _id;
    }

    void buffer_data(const void *, size_t, GLenum usage);

    void buffer_sub_data(const void *, size_t size, size_t offset);

    void set_buffer_base(int);

    GLuint get_buffer_base() const;

private:
    GLuint _id = 0;
    size_t tracked_size = 0;
    GLuint _base_index = 0;
};
} // namespace gl
