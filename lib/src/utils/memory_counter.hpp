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

void update_wait_time(float);
float get_wait_time();

void update_copy_time(float);
float get_copy_time();

void update_map_time(float);
float get_map_time();

void update_flush_time(float);
float get_flush_time();
} // namespace utils
