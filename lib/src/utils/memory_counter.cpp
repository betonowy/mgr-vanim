#include "memory_counter.hpp"

#include <mutex>

namespace
{
std::mutex _mtx;
size_t _gpu_memory = 0;
size_t _gpu_memory_peak_high = 0;
size_t _gpu_memory_peak_low = 0;
float _gpu_fence_time = 1;
float _gpu_copy_time = 1;
float _gpu_map_time = 1;
float _gpu_flush_time = 1;
} // namespace

namespace utils
{
size_t gpu_buffer_memory_peak_low()
{
    std::lock_guard lock(_mtx);
    return _gpu_memory_peak_low;
}

size_t gpu_buffer_memory_peak_high()
{
    std::lock_guard lock(_mtx);
    return _gpu_memory_peak_high;
}

void gpu_buffer_memory_peak_reset()
{
    std::lock_guard lock(_mtx);
    _gpu_memory_peak_high = _gpu_memory;
    _gpu_memory_peak_low = _gpu_memory;
}

size_t gpu_buffer_memory_used()
{
    std::lock_guard lock(_mtx);
    return _gpu_memory;
}

void gpu_buffer_memory_allocated(size_t size)
{
    std::lock_guard lock(_mtx);
    _gpu_memory += size;
    _gpu_memory_peak_high = std::max(_gpu_memory, _gpu_memory_peak_high);
}

void gpu_buffer_memory_deallocated(size_t size)
{
    std::lock_guard lock(_mtx);
    _gpu_memory -= size;
    _gpu_memory_peak_low = std::min(_gpu_memory, _gpu_memory_peak_low);
}

void update_wait_time(float time)
{
    std::lock_guard lock(_mtx);
    _gpu_fence_time = _gpu_fence_time * 0.5f + time * 0.5f;
}

float get_wait_time()
{
    std::lock_guard lock(_mtx);
    return _gpu_fence_time;
}

void update_copy_time(float time)
{
    std::lock_guard lock(_mtx);
    _gpu_copy_time = _gpu_copy_time * 0.5f + time * 0.5f;
}

float get_copy_time()
{
    std::lock_guard lock(_mtx);
    return _gpu_copy_time;
}

void update_map_time(float time)
{
    std::lock_guard lock(_mtx);
    _gpu_map_time = _gpu_map_time * 0.5f + time * 0.5f;
}

float get_map_time()
{
    std::lock_guard lock(_mtx);
    return _gpu_map_time;
}

void update_flush_time(float time)
{
    std::lock_guard lock(_mtx);
    _gpu_flush_time = _gpu_flush_time * 0.5f + time * 0.5f;
}

float get_flush_time()
{
    std::lock_guard lock(_mtx);
    return _gpu_flush_time;
}

// size_t vdb_memory_used();
} // namespace utils
