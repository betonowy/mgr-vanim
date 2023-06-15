#include "dct.hpp"

#include <immintrin.h>

#include "statistics.hpp"
#include "transform.hpp"

#include <algorithm>
#include <cstring>
#include <iterator>

namespace dvdb
{
alignas(__m256) dct_tables_888<cube_888_f32> dct_1d_f32;
alignas(__m256) dct_tables_888<cube_888_i8> dct_1d_i8;
alignas(__m256) dct_tables_888<cube_888_f32> dct_3d_f32;
alignas(__m256) dct_tables_888<cube_888_i8> dct_3d_i8;

void dct_encode_cell_f32(const cube_888_f32 *src, float *wgt, const cube_888_f32 *dct)
{
    __m256 ymm_partial_wgt = _mm256_setzero_ps();

#pragma GCC unroll 64
    for (int i = 0; i < std::size(src->values); i += 8)
    {
        __m256 ymm_src = _mm256_load_ps(src->values + i);
        __m256 ymm_dct = _mm256_load_ps(dct->values + i);

        ymm_partial_wgt = _mm256_fmadd_ps(ymm_src, ymm_dct, ymm_partial_wgt);
    }

    ymm_partial_wgt = _mm256_hadd_ps(ymm_partial_wgt, ymm_partial_wgt);

    __m128 xmm_partial_sum_low = _mm256_extractf128_ps(ymm_partial_wgt, 0);
    __m128 xmm_partial_sum_high = _mm256_extractf128_ps(ymm_partial_wgt, 1);
    __m128 xmm_partial_wgt = _mm_add_ps(xmm_partial_sum_low, xmm_partial_sum_high);

    static constexpr float reciprocal = 1.f / 512.f;
    __m128 xmm_reciprocal = _mm_set1_ps(reciprocal);

    __m128 xmm_wgt = _mm_hadd_ps(xmm_partial_wgt, xmm_partial_wgt);
    xmm_wgt = _mm_mul_ss(xmm_wgt, xmm_reciprocal);

    _mm_store_ss(wgt, xmm_wgt);
}

void dct_accumulate_decode_cell(float wgt, const cube_888_f32 *dct, cube_888_f32 *dst)
{
    __m256 ymm_wgt = _mm256_set1_ps(wgt);

#pragma GCC unroll 64
    for (int i = 0; i < std::size(dst->values); i += 8)
    {
        __m256 ymm_dct = _mm256_load_ps(dct->values + i);
        __m256 ymm_dst = _mm256_load_ps(dst->values + i);

        ymm_dst = _mm256_fmadd_ps(ymm_wgt, ymm_dct, ymm_dst);

        _mm256_store_ps(dst->values + i, ymm_dst);
    }
}

void dct_3d_encode(const cube_888_f32 *src, cube_888_f32 *dst)
{
    for (int i = 0; i < std::size(dct_3d_f32.tables); ++i)
    {
        dct_encode_cell_f32(src, dst->values + i, dct_3d_f32.tables + i);
    }
}

void dct_1d_encode(const cube_888_f32 *src, cube_888_f32 *dst)
{
    for (int i = 0; i < std::size(dct_1d_f32.tables); ++i)
    {
        dct_encode_cell_f32(src, dst->values + i, dct_1d_f32.tables + i);
    }
}

void dct_3d_decode(const cube_888_f32 *dct, cube_888_f32 *out)
{
    *out = {};

    for (int i = 0; i < std::size(dct_3d_f32.tables); ++i)
    {
        float weight = dct->values[i];

        if (weight == 0.f)
        {
            continue;
        }

        dct_accumulate_decode_cell(dct->values[i], dct_3d_f32.tables + i, out);
    }
}

void dct_1d_decode(const cube_888_f32 *dct, cube_888_f32 *out)
{
    *out = {};

    for (int i = 0; i < std::size(dct_1d_f32.tables); ++i)
    {
        float weight = dct->values[i];

        if (weight == 0.f)
        {
            continue;
        }

        dct_accumulate_decode_cell(dct->values[i], dct_1d_f32.tables + i, out);
    }
}

void dct_optimize(cube_888_f32 *dct, float max_mse, int max_tables, void (*decoder)(const cube_888_f32 *, cube_888_f32 *))
{
    max_mse *= max_mse; // as our function is unsquared

    struct weight_index
    {
        uint32_t index;
        float value;
    } weights[512];

    for (int i = 0; i < std::size(weights); ++i)
    {
        weights[i].index = i;
        weights[i].value = dct->values[i];
    }

    cube_888_f32 reference{};
    decoder(dct, &reference);

    max_tables = std::min(max_tables, static_cast<decltype(max_tables)>(std::size(weights)));
    max_tables = std::max(max_tables, 1);

    std::sort(std::begin(weights), std::end(weights), [](weight_index &lhs, weight_index &rhs) {
        return std::abs(lhs.value) > std::abs(rhs.value);
    });

    for (int i = max_tables; i < std::size(weights); ++i)
    {
        weights[i].value = 0.f;
    }

    auto transfer_weights = [&]() {
        for (int i = 0; i < std::size(weights); ++i)
        {
            dct->values[weights[i].index] = weights[i].value;
        }
    };

    transfer_weights();

    if (max_mse == 0.f)
    {
        return;
    }

    int reduction = max_tables;

    cube_888_f32 saved_dct;
    std::memcpy(&saved_dct, dct, sizeof(saved_dct));

    while (--reduction >= 0)
    {
        weights[reduction].value = 0.f;

        transfer_weights();

        cube_888_f32 trial;
        decoder(dct, &trial);
        float error = mean_squared_error(&reference, &trial);

        if (error > max_mse)
        {
            std::memcpy(dct, &saved_dct, sizeof(saved_dct));
            return;
        }

        std::memcpy(&saved_dct, dct, sizeof(saved_dct));
    }
}

void dct_3d_optimize(cube_888_f32 *dct, float max_mse, int max_tables)
{
    dct_optimize(dct, max_mse, max_tables, dct_3d_decode);
}

void dct_1d_optimize(cube_888_f32 *dct, float max_mse, int max_tables)
{
    dct_optimize(dct, max_mse, max_tables, dct_1d_decode);
}

namespace
{
void dct_1d_init_cube_f32(cube_888_f32 *dst, int x)
{
    static constexpr float reciprocal = std::numbers::pi_v<float> / 512.f;
    const float step_x = x * reciprocal;
    const float factor_x = x == 0 ? 1.f : std::numbers::sqrt2_v<float>;

    for (int i = 0; i < std::size(dst->values); ++i)
    {
        dst->values[i] = std::cos(step_x * (i + 0.5f)) * factor_x;
    }
}

void dct_3d_init_cube_f32(cube_888_f32 *dst, int x, int y, int z)
{
    static constexpr float reciprocal = std::numbers::pi_v<float> / 8.f;

    const float step_x = x * reciprocal;
    const float step_y = y * reciprocal;
    const float step_z = z * reciprocal;

    const float factor_x = x == 0 ? 1.f : std::numbers::sqrt2_v<float>;
    const float factor_y = y == 0 ? 1.f : std::numbers::sqrt2_v<float>;
    const float factor_z = z == 0 ? 1.f : std::numbers::sqrt2_v<float>;

    for (int i = 0; i < std::size(dst->values); ++i)
    {
        int cx = (0b000000111 & i) >> 0;
        int cy = (0b000111000 & i) >> 3;
        int cz = (0b111000000 & i) >> 6;

        float fz = step_z * (cz + 0.5f);
        float fy = step_y * (cy + 0.5f);
        float fx = step_x * (cx + 0.5f);

        dst->values[i] = std::cos(fz) * std::cos(fy) * std::cos(fx) * factor_x * factor_y * factor_z;
    }
}

void dct_f32_init()
{
    for (int i = 0; i < std::size(dct_3d_f32.tables); ++i)
    {
        int cti = hilbert_curve_indices.values[i];

        int x = (0b000000111 & cti) >> 0;
        int y = (0b000111000 & cti) >> 3;
        int z = (0b111000000 & cti) >> 6;

        dct_1d_init_cube_f32(dct_1d_f32.tables + i, i);
        dct_3d_init_cube_f32(dct_3d_f32.tables + i, x, y, z);
    }
}

void dct_convert_cube_f32_to_i8(const cube_888_f32 *src, cube_888_i8 *dst)
{
    for (int i = 0; i < std::size(src->values); ++i)
    {
        float v = src->values[i];

        static constexpr float sa = -1;
        static constexpr float sb = 1;

        static constexpr float da = -128;
        static constexpr float db = 127;

        dst->values[i] = (v - sa) * ((db - da) / (sb - sa)) + da;
    }
}

void dct_i8_init_from_f32()
{
    for (int i = 0; i < std::size(dct_1d_f32.tables); ++i)
    {
        const auto src_table = dct_1d_f32.tables + i;
        const auto dst_table = dct_1d_i8.tables + i;

        dct_convert_cube_f32_to_i8(src_table, dst_table);
    }

    for (int i = 0; i < std::size(dct_3d_f32.tables); ++i)
    {
        const auto src_table = dct_3d_f32.tables + i;
        const auto dst_table = dct_3d_i8.tables + i;

        dct_convert_cube_f32_to_i8(src_table, dst_table);
    }
}

void dct_3d_init_values()
{
    dct_f32_init();
    dct_i8_init_from_f32();
}
} // namespace

void dct_init()
{
    transform_init();
    dct_3d_init_values();
}
} // namespace dvdb
