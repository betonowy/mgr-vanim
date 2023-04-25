#pragma once

#include <GL/gl3w.h>

#include <cstddef>

namespace gl
{
class shader_storage
{
public:
    shader_storage()
    {
        glCreateBuffers(1, &_id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    ~shader_storage()
    {
        if (_id)
        {
            glDeleteBuffers(1, &_id);
        }
    }

    operator GLuint() const
    {
        return _id;
    }

private:
    GLuint _id = 0;
};
} // namespace gl
