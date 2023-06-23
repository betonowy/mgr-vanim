#include "gpu_memcpy.hpp"

#include <cstring>

#include <immintrin.h>

namespace utils
{
void gpu_memcpy(void *dst, const void *src, size_t size)
{
    std::memcpy(dst, src, size);

    // Don't be too smart!

    // static constexpr auto ALIGNMENT = 16;
    // size_t aligned_size = size - size & (ALIGNMENT - 1);

    // // not really better
    // for (size_t i = 0; i < aligned_size; i += ALIGNMENT)
    // {
    //     auto data = _mm256_stream_load_si256((const __m256i *)((const char *)src + i));
    //     _mm256_stream_si256((__m256i *)((char *)src + i), data);
    // }

    // std::memcpy((char *)dst + aligned_size, (const char *)src + aligned_size, size - aligned_size);
}
} // namespace utils
