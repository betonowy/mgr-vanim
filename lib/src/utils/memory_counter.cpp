#include "memory_counter.hpp"

#include <mutex>

namespace
{
std::mutex _mtx;
size_t _gpu_memory = 0;
size_t _gpu_memory_peak_high = 0;
size_t _gpu_memory_peak_low = 0;
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

// size_t vdb_memory_used();
} // namespace utils
