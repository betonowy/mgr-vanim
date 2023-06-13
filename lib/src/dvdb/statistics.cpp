#include "statistics.hpp"
#include "common.hpp"

#include <immintrin.h>
#include <iterator>

namespace dvdb
{
float mean_squared_error(const cube_888_f32 *a, const cube_888_f32 *b)
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

float mean_squared_error_with_mask(const cube_888_f32 *a, const cube_888_f32 *b, const cube_888_f32 *mask)
{
    static constexpr float reciprocal = 1.f / 512.f;

    __m256 ymm_mse = _mm256_setzero_ps();

#pragma GCC unroll 64
    for (int i = 0; i < std::size(a->values); i += 8)
    {
        __m256 ymm_mask = _mm256_loadu_ps(mask->values + i);
        __m256 ymm_a = _mm256_loadu_ps(a->values + i);
        __m256 ymm_b = _mm256_loadu_ps(b->values + i);
        __m256 ymm_diff = _mm256_mul_ps(_mm256_sub_ps(ymm_a, ymm_b), ymm_mask);

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

float accumulate(const cube_888_f32 *src)
{
    __m256 ymm_acc = _mm256_setzero_ps();

#pragma GCC unroll 64
    for (int i = 0; i < std::size(src->values); i += 8)
    {
        __m256 ymm_src = _mm256_loadu_ps(src->values + i);
        ymm_acc = _mm256_add_ps(ymm_acc, ymm_src);
    }

    ymm_acc = _mm256_hadd_ps(ymm_acc, ymm_acc);

    __m128 xmm_acc_low = _mm256_extractf128_ps(ymm_acc, 0);
    __m128 xmm_acc_high = _mm256_extractf128_ps(ymm_acc, 1);

    __m128 xmm_acc = _mm_add_ps(xmm_acc_low, xmm_acc_high);

    xmm_acc = _mm_hadd_ps(xmm_acc, xmm_acc);

    float acc;

    _mm_store_ss(&acc, xmm_acc);

    return acc;
}

float mean(const cube_888_f32 *src)
{
    return accumulate(src) * (1.f / 512.f);
}

void linear_regression(const cube_888_f32 *x, const cube_888_f32 *y, float *a, float *b)
{
    float x_mean = mean(x);
    float y_mean = mean(y);

    cube_888_f32 tmp;

    mul(x, y, &tmp);
    float ss_xy = accumulate(&tmp) - std::size(tmp.values) * x_mean * y_mean;

    mul(x, x, &tmp);
    float ss_xx = accumulate(&tmp) - std::size(tmp.values) * x_mean * x_mean;

    *b = ss_xy / ss_xx;
    *a = y_mean - *b * x_mean;
}
} // namespace dvdb
