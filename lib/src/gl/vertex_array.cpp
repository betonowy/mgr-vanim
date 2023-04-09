#include "vertex_array.hpp"

namespace gl
{
vertex_array::vertex_array()
{
    glCreateVertexArrays(1, &_id);
}

vertex_array::~vertex_array()
{
    if (_id)
    {
        glDeleteVertexArrays(1, &_id);
    }
}

void vertex_array::setup(std::initializer_list<attribute> attributes, std::shared_ptr<vertex_buffer> buffer)
{
    glBindVertexArray(_id);
    {
        glBindBuffer(GL_ARRAY_BUFFER, *buffer);
        for (const auto &attr : attributes)
        {
            glVertexAttribPointer(attr.index, attr.size, attr.type, attr.normalized, attr.stride, attr.offset);
            glEnableVertexAttribArray(attr.index);
        }
    }
    glBindVertexArray(0);

    _associated_buffer = std::move(buffer);
}
} // namespace gl
