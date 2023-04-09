#include "vertex_buffer.hpp"

#include <utils/memory_counter.hpp>

namespace gl
{
vertex_buffer::vertex_buffer()
{
    glCreateBuffers(1, &_id);
    glBindBuffer(GL_ARRAY_BUFFER, _id);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

vertex_buffer::~vertex_buffer()
{
    if (_id)
    {
        utils::gpu_buffer_memory_deallocated(tracked_size);
        glDeleteVertexArrays(1, &_id);
    }
}

void vertex_buffer::buffer_data(void *data, size_t size, GLenum usage)
{
    utils::gpu_buffer_memory_deallocated(tracked_size);
    {
        glNamedBufferData(_id, size, data, usage);
        tracked_size = size;
    }
    utils::gpu_buffer_memory_allocated(tracked_size);
}
} // namespace gl
