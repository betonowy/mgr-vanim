#include "common.hpp"

#include <immintrin.h>

#include <iterator>

namespace dvdb
{
void add(const cube_888_f32 *lhs, const cube_888_f32 *rhs, cube_888_f32 *dst)
{
    for (int i = 0; i < std::size(lhs->values); i += 8)
    {
        __m256 ymm_a = _mm256_loadu_ps(lhs->values + i);
        __m256 ymm_b = _mm256_loadu_ps(lhs->values + i);

        __m256 ymm_dst = _mm256_add_ps(ymm_a, ymm_b);

        _mm256_storeu_ps(dst->values + i, ymm_dst);
    }
}

void sub(const cube_888_f32 *lhs, const cube_888_f32 *rhs, cube_888_f32 *dst)
{
    for (int i = 0; i < std::size(lhs->values); i += 8)
    {
        __m256 ymm_a = _mm256_loadu_ps(lhs->values + i);
        __m256 ymm_b = _mm256_loadu_ps(lhs->values + i);

        __m256 ymm_dst = _mm256_sub_ps(ymm_a, ymm_b);

        _mm256_storeu_ps(dst->values + i, ymm_dst);
    }
}

void mul(const cube_888_f32 *lhs, const cube_888_f32 *rhs, cube_888_f32 *dst)
{
    for (int i = 0; i < std::size(lhs->values); i += 8)
    {
        __m256 ymm_a = _mm256_loadu_ps(lhs->values + i);
        __m256 ymm_b = _mm256_loadu_ps(lhs->values + i);

        __m256 ymm_dst = _mm256_mul_ps(ymm_a, ymm_b);

        _mm256_storeu_ps(dst->values + i, ymm_dst);
    }
}

void div(const cube_888_f32 *lhs, const cube_888_f32 *rhs, cube_888_f32 *dst)
{
    for (int i = 0; i < std::size(lhs->values); i += 8)
    {
        __m256 ymm_a = _mm256_loadu_ps(lhs->values + i);
        __m256 ymm_b = _mm256_loadu_ps(lhs->values + i);

        __m256 ymm_dst = _mm256_div_ps(ymm_a, ymm_b);

        _mm256_storeu_ps(dst->values + i, ymm_dst);
    }
}

void div_fast(const cube_888_f32 *lhs, const cube_888_f32 *rhs, cube_888_f32 *dst)
{
    for (int i = 0; i < std::size(lhs->values); i += 8)
    {
        __m256 ymm_a = _mm256_loadu_ps(lhs->values + i);
        __m256 ymm_b = _mm256_loadu_ps(lhs->values + i);

        __m256 ymm_b_rcp = _mm256_rcp_ps(ymm_b);
        __m256 ymm_dst = _mm256_mul_ps(ymm_a, ymm_b_rcp);

        _mm256_storeu_ps(dst->values + i, ymm_dst);
    }
}

void rcp(const cube_888_f32 *src, cube_888_f32 *dst)
{
    for (int i = 0; i < std::size(src->values); i += 8)
    {
        __m256 ymm_a = _mm256_loadu_ps(src->values + i);
        __m256 ymm_dst = _mm256_rcp_ps(ymm_a);

        _mm256_storeu_ps(dst->values + i, ymm_dst);
    }
}

void rsqrt(const cube_888_f32 *src, cube_888_f32 *dst)
{
    for (int i = 0; i < std::size(src->values); i += 8)
    {
        __m256 ymm_a = _mm256_loadu_ps(src->values + i);
        __m256 ymm_dst = _mm256_rsqrt_ps(ymm_a);

        _mm256_storeu_ps(dst->values + i, ymm_dst);
    }
}

void sqrt(const cube_888_f32 *src, cube_888_f32 *dst)
{
    for (int i = 0; i < std::size(src->values); i += 8)
    {
        __m256 ymm_a = _mm256_loadu_ps(src->values + i);
        __m256 ymm_dst = _mm256_sqrt_ps(ymm_a);

        _mm256_storeu_ps(dst->values + i, ymm_dst);
    }
}
} // namespace dvdb
