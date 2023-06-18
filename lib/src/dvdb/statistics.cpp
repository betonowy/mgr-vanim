#include "statistics.hpp"
#include "common.hpp"

#include <immintrin.h>
#include <iterator>

namespace dvdb
{
namespace
{
float accumulate_register_256(__m256 ymm)
{
    ymm = _mm256_hadd_ps(ymm, ymm);

    __m128 xmm_low = _mm256_extractf128_ps(ymm, 0);
    __m128 xmm_high = _mm256_extractf128_ps(ymm, 1);

    __m128 xmm = _mm_add_ps(xmm_low, xmm_high);

    xmm = _mm_hadd_ps(xmm, xmm);

    float ret;

    _mm_store_ss(&ret, xmm);

    return ret;
}
} // namespace

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

    return accumulate_register_256(ymm_mse) * reciprocal;
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

    return accumulate_register_256(ymm_mse) * reciprocal;
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

    return accumulate_register_256(ymm_acc);
}

float mean(const cube_888_f32 *src)
{
    return accumulate(src) * (1.f / 512.f);
}

void linear_regression(const cube_888_f32 *x, const cube_888_f32 *y, float *add, float *mul)
{
    float x_mean = mean(x);
    float y_mean = mean(y);

    __m256 ymm_x_mean = _mm256_set1_ps(x_mean);
    __m256 ymm_y_mean = _mm256_set1_ps(y_mean);

    __m256 ymm_ss_xx = _mm256_setzero_ps();
    __m256 ymm_ss_xy = _mm256_setzero_ps();

#pragma GCC unroll 64
    for (int i = 0; i < std::size(x->values); i += 8)
    {
        __m256 ymm_x = _mm256_loadu_ps(x->values + i);
        __m256 ymm_y = _mm256_loadu_ps(y->values + i);

        __m256 ymm_diff_x = _mm256_sub_ps(ymm_x, ymm_x_mean);
        __m256 ymm_diff_y = _mm256_sub_ps(ymm_y, ymm_y_mean);

        __m256 ymm_diff_xx = _mm256_mul_ps(ymm_diff_x, ymm_diff_x);
        __m256 ymm_diff_xy = _mm256_mul_ps(ymm_diff_x, ymm_diff_y);

        ymm_ss_xx = _mm256_add_ps(ymm_ss_xx, ymm_diff_xx);
        ymm_ss_xy = _mm256_add_ps(ymm_ss_xy, ymm_diff_xy);
    }

    float ss_xx = accumulate_register_256(ymm_ss_xx);
    float ss_xy = accumulate_register_256(ymm_ss_xy);

    *mul = ss_xx == 0 ? 0 : ss_xy / ss_xx;
    *add = y_mean - *mul * x_mean;
}

void linear_regression_with_mask(const cube_888_f32 *x, const cube_888_f32 *y, float *add, float *mul, const cube_888_f32 *mask)
{
    cube_888_f32 masked_x, masked_y;

    dvdb::mul(x, mask, &masked_x);
    dvdb::mul(y, mask, &masked_y);

    float x_mean = mean(&masked_x);
    float y_mean = mean(&masked_y);

    __m256 ymm_x_mean = _mm256_set1_ps(x_mean);
    __m256 ymm_y_mean = _mm256_set1_ps(y_mean);

    __m256 ymm_ss_xx = _mm256_setzero_ps();
    __m256 ymm_ss_xy = _mm256_setzero_ps();

#pragma GCC unroll 64
    for (int i = 0; i < std::size(masked_x.values); i += 8)
    {
        __m256 ymm_x = _mm256_loadu_ps(masked_x.values + i);
        __m256 ymm_y = _mm256_loadu_ps(masked_y.values + i);

        __m256 ymm_diff_x = _mm256_sub_ps(ymm_x, ymm_x_mean);
        __m256 ymm_diff_y = _mm256_sub_ps(ymm_y, ymm_y_mean);

        __m256 ymm_diff_xx = _mm256_mul_ps(ymm_diff_x, ymm_diff_x);
        __m256 ymm_diff_xy = _mm256_mul_ps(ymm_diff_x, ymm_diff_y);

        ymm_ss_xx = _mm256_add_ps(ymm_ss_xx, ymm_diff_xx);
        ymm_ss_xy = _mm256_add_ps(ymm_ss_xy, ymm_diff_xy);
    }

    float ss_xx = accumulate_register_256(ymm_ss_xx);
    float ss_xy = accumulate_register_256(ymm_ss_xy);

    *mul = ss_xx == 0 ? 0 : ss_xy / ss_xx;
    *add = y_mean - *mul * x_mean;
}
} // namespace dvdb
