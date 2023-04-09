#include "shader_storage.hpp"

#include <utils/memory_counter.hpp>

namespace gl
{
shader_storage::shader_storage()
{
    glCreateBuffers(1, &_id);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

shader_storage::~shader_storage()
{
    if (_id)
    {
        utils::gpu_buffer_memory_deallocated(tracked_size);
        glDeleteBuffers(1, &_id);
    }
}

void shader_storage::buffer_data(const void *data, size_t size, GLenum usage)
{
    utils::gpu_buffer_memory_deallocated(tracked_size);
    {
        glNamedBufferData(_id, size, data, usage);
        tracked_size = size;
    }
    utils::gpu_buffer_memory_allocated(tracked_size);
}

void shader_storage::buffer_sub_data(const void *data, size_t size, size_t offset)
{
    glNamedBufferSubData(_id, offset, size, data);
}

void shader_storage::set_buffer_base(int index)
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, _id);
    _base_index = index;
}

unsigned shader_storage::get_buffer_base() const
{
    return _base_index;
}
} // namespace gl
