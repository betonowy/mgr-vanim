#pragma once

#include <cstddef>

namespace utils
{
void gpu_memcpy(void *dst, const void *src, size_t size);
}
