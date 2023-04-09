#pragma once

#include <GL/gl3w.h>

#include "vertex_buffer.hpp"

#include <initializer_list>
#include <memory>

namespace gl
{
class vertex_array
{
public:
    vertex_array();
    ~vertex_array();

    vertex_array(const vertex_array &) = delete;
    vertex_array &operator=(const vertex_array &) = delete;
    vertex_array(vertex_array &&) = delete;
    vertex_array &operator=(vertex_array &&) = delete;

    operator GLuint() const
    {
        return _id;
    }

    struct attribute
    {
        int index;
        int size;
        GLenum type;
        bool normalized;
        int stride;
        const void *offset = nullptr;
    };

    void setup(std::initializer_list<attribute>, std::shared_ptr<vertex_buffer>);

private:
    GLuint _id = 0;
    std::shared_ptr<vertex_buffer> _associated_buffer;
};
} // namespace gl
