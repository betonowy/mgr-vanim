#include "fma.hpp"

#include "statistics.hpp"

#include <immintrin.h>
#include <iterator>

namespace dvdb
{
void find_fused_multiply_add(const cube_888_f32 *src, const cube_888_f32 *dst, float *add, float *mul)
{
    *add = 0.f;
    *mul = 1.f;

    for (int i = 0; i < std::size(src->values); ++i)
    {
        if (!std::isfinite(src->values[i]) || !std::isfinite(dst->values[i]))
        {
            return;
        }
    }

    static constexpr int MAX_ITERATIONS = 1000;
    static constexpr float MIN_STEP = 1e-12;
    static constexpr float STEP_FAIL_MULTIPLIER = 0.33f;
    static constexpr float STEP_SUCCESS_MULTIPLIER = 1.33f;

    cube_888_f32 tmp;

    float mul_step = 0.5f;
    float add_step = 0.5f;

    bool mul_done = false;
    bool add_done = false;

    for (int i = 0; i < MAX_ITERATIONS; ++i)
    {
        // multiplication learning
        {
            float up_mul = std::max(*mul + mul_step, std::nextafter(*mul, +INFINITY));
            float down_mul = std::min(*mul - mul_step, std::nextafter(*mul, -INFINITY));

            fused_multiply_add(src, &tmp, *add, *mul);
            float base_error = mean_squared_error(dst, &tmp);

            fused_multiply_add(src, &tmp, *add, up_mul);
            float up_error = mean_squared_error(dst, &tmp);

            fused_multiply_add(src, &tmp, *add, down_mul);
            float down_error = mean_squared_error(dst, &tmp);

            if (base_error > up_error || base_error > down_error)
            {
                *mul = down_error < up_error ? down_mul : up_mul;

                mul_done = false;
                mul_step *= STEP_SUCCESS_MULTIPLIER;
            }
            else
            {
                mul_done = MIN_STEP > mul_step;
                mul_step *= STEP_FAIL_MULTIPLIER;
            }
        }

        // addition learning
        {
            float up_add = std::max(*add + add_step, std::nextafter(*add, +INFINITY));
            float down_add = std::min(*add - add_step, std::nextafter(*add, -INFINITY));

            fused_multiply_add(src, &tmp, *add, *mul);
            float base_error = mean_squared_error(dst, &tmp);

            fused_multiply_add(src, &tmp, up_add, *mul);
            float up_error = mean_squared_error(dst, &tmp);

            fused_multiply_add(src, &tmp, down_add, *mul);
            float down_error = mean_squared_error(dst, &tmp);

            if (base_error > up_error || base_error > down_error)
            {
                *add = down_error < up_error ? down_add : up_add;

                add_done = false;
                add_step *= STEP_SUCCESS_MULTIPLIER;
            }
            else
            {
                add_done = MIN_STEP > add_step;
                add_step *= STEP_FAIL_MULTIPLIER;
            }
        }

        if (add_done && mul_done)
        {
            break;
        }
    }
}

void fused_multiply_add(const cube_888_f32 *src, cube_888_f32 *dst, float add, float mul)
{
    __m256 ymm_add = _mm256_set1_ps(add);
    __m256 ymm_mul = _mm256_set1_ps(mul);

    for (int i = 0; i < std::size(src->values); i += 8)
    {
        __m256 ymm_src = _mm256_loadu_ps(src->values + i);
        __m256 ymm_dst = _mm256_fmadd_ps(ymm_src, ymm_mul, ymm_add);
        _mm256_storeu_ps(dst->values + i, ymm_dst);
    }
}
} // namespace dvdb
