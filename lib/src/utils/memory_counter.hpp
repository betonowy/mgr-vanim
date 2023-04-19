#pragma once

#include <cstddef>

namespace utils
{
size_t gpu_buffer_memory_used();
size_t gpu_buffer_memory_peak_low();
size_t gpu_buffer_memory_peak_high();
void gpu_buffer_memory_peak_reset();

void gpu_buffer_memory_allocated(size_t);
void gpu_buffer_memory_deallocated(size_t);

// size_t vdb_memory_used();
} // namespace utils
