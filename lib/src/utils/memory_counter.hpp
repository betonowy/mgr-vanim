#pragma once

#include <cstddef>

namespace utils
{
size_t gpu_buffer_memory_used();
void gpu_buffer_memory_allocated(size_t);
void gpu_buffer_memory_deallocated(size_t);

// size_t vdb_memory_used();
} // namespace utils
