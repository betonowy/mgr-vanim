#include "diff.hpp"

#include "immintrin.h"

#include <iterator>

namespace dvdb
{
float mse(const cube_888_f32 *a, const cube_888_f32 *b)
{
    static constexpr float reciprocal = 1.f / 512.f;

    __m256 ymm_mse = _mm256_setzero_ps();

#pragma GCC unroll 64
    for (int i = 0; i < std::size(a->values); i += 8)
    {
        __m256 ymm_a = _mm256_loadu_ps(a->values + i);
        __m256 ymm_b = _mm256_loadu_ps(b->values + i);
        __m256 ymm_diff = _mm256_sub_ps(ymm_a, ymm_b);

        ymm_mse = _mm256_add_ps(ymm_mse, _mm256_mul_ps(ymm_diff, ymm_diff));
    }

    ymm_mse = _mm256_hadd_ps(ymm_mse, ymm_mse);

    __m128 xmm_mse_low = _mm256_extractf128_ps(ymm_mse, 0);
    __m128 xmm_mse_high = _mm256_extractf128_ps(ymm_mse, 1);

    __m128 xmm_mse = _mm_add_ps(xmm_mse_low, xmm_mse_high);

    xmm_mse = _mm_hadd_ps(xmm_mse, xmm_mse);

    float mse;

    _mm_store_ss(&mse, xmm_mse);

    return mse * reciprocal;
}
} // namespace dvdb
