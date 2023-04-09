#include "memory_counter.hpp"

#include <mutex>

namespace
{
std::mutex _mtx;
size_t gpu_memory = 0;
} // namespace

namespace utils
{

size_t gpu_buffer_memory_used()
{
    std::lock_guard lock(_mtx);
    return gpu_memory;
}

void gpu_buffer_memory_allocated(size_t size)
{
    std::lock_guard lock(_mtx);
    gpu_memory += size;
}

void gpu_buffer_memory_deallocated(size_t size)
{
    std::lock_guard lock(_mtx);
    gpu_memory -= size;
}

// size_t vdb_memory_used();
} // namespace utils
