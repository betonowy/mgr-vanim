#include "uniform_buffer.hpp"

#include <utils/memory_counter.hpp>

namespace gl
{
uniform_buffer::uniform_buffer()
{
    glCreateBuffers(1, &_id);
    glBindBuffer(GL_UNIFORM_BUFFER, _id);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

uniform_buffer::~uniform_buffer()
{
    if (_id)
    {
        utils::gpu_buffer_memory_deallocated(_tracked_size);
        glDeleteBuffers(1, &_id);
    }
}

void uniform_buffer::buffer_data(const void *data, size_t size, GLenum usage)
{
    utils::gpu_buffer_memory_deallocated(_tracked_size);
    {
        glNamedBufferData(_id, size, data, usage);
        _tracked_size = size;
    }
    utils::gpu_buffer_memory_allocated(_tracked_size);
}

void uniform_buffer::set_buffer_base(int index)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, index, _id);
    _base_index = index;
}

unsigned uniform_buffer::get_buffer_base() const
{
    return _base_index;
}
} // namespace gl
