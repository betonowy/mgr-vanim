#include "dvdb_benchmark_cases.hpp"

#include <immintrin.h>

#include <cmath>
#include <cstdint>
#include <numbers>

#define align256 alignas(alignof(__m256))

namespace cases
{
void copy_for_loop_scl(const float *dvdb_restrict input, float *dvdb_restrict output, const float *dvdb_restrict work_group)
{
#pragma GCC unroll 4
    for (int i = 0; i < INTERFACE_SIZE; ++i)
    {
        output[i] = input[i] * work_group[i];
    }
}

void copy_for_loop_avx(const float *dvdb_restrict input, float *dvdb_restrict output, const float *dvdb_restrict work_group)
{
#pragma GCC unroll 64
    for (int i = 0; i < INTERFACE_SIZE; i += 8)
    {
        __m256 ymm_i = _mm256_load_ps(input + i);
        __m256 ymm_w = _mm256_load_ps(work_group + i);
        _mm256_store_ps(output + i, _mm256_mul_ps(ymm_i, ymm_w));
    }
}

void cube_value_offset_scl(const float *input, float *output, int ox, int oy, int oz)
{
    ox &= 7;
    oy &= 7;
    oz &= 7;

    auto cube8index = [](unsigned x, unsigned y, unsigned z) {
        return x + 8 * y + 8 * 8 * z;
    };

    auto cube8index_wrapped = [](unsigned x, unsigned y, unsigned z) {
        return ((x & 7) + 8 * (y & 7) + 8 * 8 * (z & 7));
    };

    for (int z = 0; z < 8; ++z)
    {
        for (int y = 0; y < 8; ++y)
        {
            for (int x = 0; x < 8; ++x)
            {
                output[cube8index_wrapped(x + ox, y + oy, z + oz)] = input[cube8index(x, y, z)];
            }
        }
    }
}

void cube_value_offset_avx(const float *dvdb_restrict input, float *dvdb_restrict output, int ox, int oy, int oz)
{
    ox = -ox & 0b111;
    oy &= 7;
    oz &= 7;

    alignas(__m256i) static constexpr uint32_t x_indices[8] = {0, 1, 2, 3, 4, 5, 6, 7};

    __m256i permutation;
    __m128 half_mask;

    {
        __m256i x_off = _mm256_set1_epi32(ox);
        __m256i x_idx = _mm256_load_si256(reinterpret_cast<const __m256i *>(x_indices));

        permutation = _mm256_add_epi32(x_idx, x_off);

        __m256i b2 = _mm256_set1_epi32(0b11);
        __m256i t1 = _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(permutation), _mm256_castsi256_ps(b2)));

        __m256i mask;

        if (ox <= 4 && ox != 0)
        {
            mask = _mm256_castps_si256(_mm256_cmp_ps(_mm256_castsi256_ps(x_idx), _mm256_castsi256_ps(t1), _CMP_GE_OQ));
        }
        else
        {
            mask = _mm256_castps_si256(_mm256_cmp_ps(_mm256_castsi256_ps(x_idx), _mm256_castsi256_ps(t1), _CMP_LT_OQ));
        }

        half_mask = _mm_castsi128_ps(_mm256_castsi256_si128(mask));
    }

#pragma GCC unroll 8
    for (int z = 0; z < 8; ++z)
    {
#pragma GCC unroll 8
        for (int y = 0; y < 8; ++y)
        {
            int index_in = y * 8 + z * 8 * 8;
            int index_out = ((y + oy) & 0b111) * 8 + ((z + oz) & 0b111) * 8 * 8;

            __m256 row = _mm256_loadu_ps(input + index_in);
            __m256 half_rotated = _mm256_permutevar_ps(row, permutation);

            __m128 half_rotated_low = _mm256_extractf128_ps(half_rotated, 0);
            __m128 half_rotated_high = _mm256_extractf128_ps(half_rotated, 1);

            __m128 rotated_low = _mm_blendv_ps(half_rotated_low, half_rotated_high, half_mask);
            __m128 rotated_high = _mm_blendv_ps(half_rotated_high, half_rotated_low, half_mask);

            __m256 out = _mm256_set_m128(rotated_high, rotated_low);

            _mm256_storeu_ps(output + index_out, out);
        }
    }
}

void cube_diff_scl(const float *dvdb_restrict a, const float *dvdb_restrict b, float *dvdb_restrict diff)
{
#pragma GCC unroll 32
    for (int i = 0; i < INTERFACE_SIZE; ++i)
    {
        diff[i] = a[i] - b[i];
    }
}

void cube_diff_avx(const float *dvdb_restrict a, const float *dvdb_restrict b, float *dvdb_restrict diff)
{
#pragma GCC unroll 64
    for (int i = 0; i < INTERFACE_SIZE; i += 8)
    {
        __m256 ymm_a = _mm256_loadu_ps(a + i);
        __m256 ymm_b = _mm256_loadu_ps(b + i);
        _mm256_storeu_ps(diff + i, _mm256_sub_ps(ymm_a, ymm_b));
    }
}

void cube_diff_epi8_scl(const int8_t *dvdb_restrict a, const int8_t *dvdb_restrict b, int8_t *dvdb_restrict diff)
{
#pragma GCC unroll 16
    for (int i = 0; i < INTERFACE_SIZE; ++i)
    {
        diff[i] = a[i] - b[i];
    }
}

void cube_diff_epi8_avx(const int8_t *dvdb_restrict a, const int8_t *dvdb_restrict b, int8_t *dvdb_restrict diff)
{
#pragma GCC unroll 16
    for (int i = 0; i < INTERFACE_SIZE; i += 32)
    {
        __m256i ymm_a = _mm256_loadu_si256((__m256i *)(a + i));
        __m256i ymm_b = _mm256_loadu_si256((__m256i *)(b + i));
        _mm256_storeu_si256((__m256i *)(diff + i), _mm256_sub_epi8(ymm_a, ymm_b));
    }
}

float cube_mse_scl(const float *dvdb_restrict a, const float *dvdb_restrict b)
{
    static constexpr float reciprocal = 1.f / INTERFACE_SIZE;

    float mse = 0.f;

    for (int i = 0; i < INTERFACE_SIZE; ++i)
    {
        float diff = a[i] - b[i];
        mse += diff * diff;
    }

    return mse * reciprocal;
}

float cube_mse_avx(const float *dvdb_restrict a, const float *dvdb_restrict b)
{
    static constexpr float reciprocal = 1.f / INTERFACE_SIZE;

    __m256 ymm_mse = _mm256_setzero_ps();

#pragma GCC unroll 64
    for (int i = 0; i < INTERFACE_SIZE; i += 8)
    {
        __m256 ymm_a = _mm256_loadu_ps(a + i);
        __m256 ymm_b = _mm256_loadu_ps(b + i);
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

static constexpr auto TRIG_LUT_SIZE = 128;
static float sin_lut[TRIG_LUT_SIZE];

void prepare_trig_lut()
{
    for (size_t i = 0; i < TRIG_LUT_SIZE; ++i)
    {
        sin_lut[i] = std::sin(std::numbers::pi * i / static_cast<double>(TRIG_LUT_SIZE - 1) / 2.);
    }

    sin_lut[0] = 0;
    sin_lut[TRIG_LUT_SIZE - 1] = 1;
}

float sin_scl(float x)
{
    float x_rept = x * std::numbers::inv_pi_v<float> * 0.5f;
    x_rept -= std::floor(x_rept);
    float x_part = x_rept * 4;

    int xi_part = x_part;
    int dir = xi_part & 1;
    int sig = xi_part >> 1;

    x_part -= std::floor(x_part);

    x_part = dir ? 1 - x_part : x_part;

    float i_f = x_part * (TRIG_LUT_SIZE - 1);
    int i_hi = std::ceil(i_f);
    int i_lo = std::floor(i_f);

    float ratio_hi = i_f - std::floor(i_f);
    float ratio_lo = 1.f - ratio_hi;

    float value = sin_lut[i_hi] * ratio_hi + sin_lut[i_lo] * ratio_lo;

    return sig ? -value : value;
}

void sin_avx(float *in, float *out)
{
}

void compute_cube_dct_scl(float *dvdb_restrict output, int x, int y, int z)
{
    static constexpr float reciprocal = 2.f * std::numbers::pi_v<float> / 8;

    float diff_x = x * reciprocal;
    float diff_y = y * reciprocal;
    float diff_z = z * reciprocal;

    for (int k = 0; k < 8; ++k)
    {
        for (int j = 0; j < 8; ++j)
        {
            for (int i = 0; i < 8; ++i)
            {
                float a = diff_x * (i + 0.5f);
                float b = diff_y * (j + 0.5f);
                float c = diff_z * (k + 0.5f);

                output[i + (j << 3) + (k << 6)] = std::cos(a) * std::cos(b) * std::cos(c);
            }
        }
    }
}

void compute_cube_dct_avx(float *dvdb_restrict output, int x, int y, int z)
{
    static constexpr float reciprocal = 2.f * std::numbers::pi_v<float> / 8;

    __m256 diff_x = _mm256_set1_ps(x * reciprocal);
    __m256 diff_y = _mm256_set1_ps(y * reciprocal);
    __m256 diff_z = _mm256_set1_ps(z * reciprocal);

    __m256 i_base = _mm256_set_ps(0, 1, 2, 3, 4, 5, 6, 7);
    __m256 half = _mm256_set1_ps(.5f);

    auto avx_cos = [](__m256 phi) {
        return phi;
    };

    for (int k = 0; k < 8; ++k)
    {

        for (int j = 0; j < 8; ++j)
        {
            // __m256 a = ;

            _mm256_storeu_ps(output + (j << 3) + (k << 6), half);

            for (int i = 0; i < 8; ++i)
            {
                // float a = diff_x * (i + 0.5f);
                // float b = diff_y * (j + 0.5f);
                // float c = diff_z * (k + 0.5f);

                // output[i + (j << 3) + (k << 6)] = std::cos(a) * std::cos(b) * std::cos(c);
            }
        }
    }
}
} // namespace cases
