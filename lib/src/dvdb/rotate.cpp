#include "rotate.hpp"

#include "common.hpp"
#include "statistics.hpp"

#include <cstring>
#include <immintrin.h>

namespace dvdb
{
namespace
{
constexpr auto INDEX_CENTER = 13;

void rotate_internal(const cube_888_mask *src, cube_888_mask *dst, int ox, int oy, int oz)
{
    ox = -ox & 0b111;
    oy &= 0b111;
    oz &= 0b111;

    auto cube8index = [](unsigned x, unsigned y, unsigned z) {
        return x + 8 * y + 8 * 8 * z;
    };

    auto cube8index_wrapped = [](unsigned x, unsigned y, unsigned z) {
        return ((x & 7) + 8 * (y & 7) + 8 * 8 * (z & 7));
    };

#pragma GCC unroll 8
    for (int z = 0; z < 8; ++z)
    {
#pragma GCC unroll 8
        for (int y = 0; y < 8; ++y)
        {
            auto input = ((uint8_t *)src) + cube8index(y, z, 0);
            auto output = ((uint8_t *)dst) + cube8index_wrapped(y + oy, z + oz, 0);

            uint8_t expected = *output;

            uint16_t tmp = *input;
            tmp |= tmp << 8 | tmp;
            tmp >>= ox;

            *output = tmp;
        }
    }
}

void rotate_internal(const cube_888_f32 *src, cube_888_f32 *dst, int ox, int oy, int oz)
{
    ox = -ox & 0b111;
    oy &= 0b111;
    oz &= 0b111;

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

            __m256 row = _mm256_loadu_ps(src->values + index_in);
            __m256 half_rotated = _mm256_permutevar_ps(row, permutation);

            __m128 half_rotated_low = _mm256_extractf128_ps(half_rotated, 0);
            __m128 half_rotated_high = _mm256_extractf128_ps(half_rotated, 1);

            __m128 rotated_low = _mm_blendv_ps(half_rotated_low, half_rotated_high, half_mask);
            __m128 rotated_high = _mm_blendv_ps(half_rotated_high, half_rotated_low, half_mask);

            __m256 out = _mm256_set_m128(rotated_high, rotated_low);

            _mm256_storeu_ps(dst->values + index_out, out);
        }
    }
}

constexpr int coords_to_cube_index(int x, int y, int z)
{
    return x + (y << 3) + (z << 6);
}

constexpr int coords_to_neighbor_index(int x, int y, int z)
{
    return (x + 1) + (y + 1) * 3 + (z + 1) * 3 * 3;
}

constexpr void limit_coord_active(int v, int &sb, int &se, int &db, int &de)
{
    bool cond = v > 0;

    sb = cond ? 8 - v : 0;
    se = cond ? 8 : -v;
    db = cond ? 0 : 8 + v;
    de = cond ? v : 8;
}

constexpr void limit_coord_passive(int v, int &sb, int &se, int &db, int &de)
{
    bool cond = v > 0;

    sb = cond ? 0 : -v;
    se = cond ? 8 - v : 8;
    db = cond ? v : 0;
    de = cond ? 8 : 8 + v;
}

constexpr void index_to_step_offset(int i, int *x, int *y, int *z)
{
    *x = (i / 1) % 3 - 1;
    *y = (i / 3) % 3 - 1;
    *z = (i / 9) % 3 - 1;
}

constexpr int min_value27(float values[27])
{
    int min_index = 0;
    float min_value = values[0];

    for (int i = 1; i < 27; ++i)
    {
        if (min_value > values[i])
        {
            min_value = values[i];
            min_index = i;
        }
    }

    return min_index;
}
} // namespace

consteval dvdb::cube_888_f32 filled_cube(float value)
{
    dvdb::cube_888_f32 cube;

    for (int i = 0; i < 512; ++i)
    {
        cube.values[i] = value;
    }

    return cube;
}

static constexpr dvdb::cube_888_f32 default_mask = filled_cube(1);

float rotate_refill_find_astar(const cube_888_f32 *dst, const cube_888_f32 *dst_mask, cube_888_f32 *src[27], int *x, int *y, int *z)
{
    cube_888_f32 test;
    float test_error[27];

    if (!dst_mask)
    {
        dst_mask = &default_mask;
    }

    *x = *y = *z = 0;

    while (true)
    {
        int tx, ty, tz;

        for (int i = 0; i < 27; ++i)
        {
            index_to_step_offset(i, &tx, &ty, &tz);
            rotate_refill(&test, src, tx + *x, ty + *y, tz + *z);

            float pre_fma_error = mean_squared_error_with_mask(&test, dst, dst_mask);

            float add, mul;
            linear_regression(&test, dst, &add, &mul);
            fma(&test, &test, add, mul);

            float post_fma_error = mean_squared_error_with_mask(&test, dst, dst_mask);

            test_error[i] = std::min(pre_fma_error, post_fma_error);
        }

        int i = min_value27(test_error);
        index_to_step_offset(i, &tx, &ty, &tz);

        if (tx == 0 && ty == 0 && tz == 0)
        {
            return test_error[i];
        }

        *x += tx, *y += ty, *z += tz;

        if (std::abs(*x) > 7 || std::abs(*y) > 7 || std::abs(*z) > 7)
        {
            return test_error[i];
        }
    }
}

float rotate_refill_find_brute_force(const cube_888_f32 *dst, const cube_888_f32 *dst_mask, cube_888_f32 *src[27], int *x, int *y, int *z)
{
    cube_888_f32 test;
    float best = std::numeric_limits<float>::max();

    if (!dst_mask)
    {
        dst_mask = &default_mask;
    }

    *x = *y = *z = 0;

    for (int tz = -8; tz <= 8; ++tz)
    {
        for (int ty = -8; ty <= 8; ++ty)
        {
            for (int tx = -8; tx <= 8; ++tx)
            {
                rotate_refill(&test, src, tx, ty, tz);

                float pre_fma_error = mean_squared_error_with_mask(&test, dst, dst_mask);

                float add, mul;
                linear_regression(&test, dst, &add, &mul);
                fma(&test, &test, add, mul);

                float post_fma_error = mean_squared_error_with_mask(&test, dst, dst_mask);

                float error = std::min(pre_fma_error, post_fma_error);

                if (error < best)
                {
                    *x = tx, *y = ty, *z = tz, best = error;
                }
            }
        }
    }

    return best;
}

template <int AX, int AY, int AZ, typename T>
void range_copy(T *dst, const T *src, int x, int y, int z)
{
    int sbx, sex, dbx, dex, sby, sey, dby, dey, sbz, sez, dbz, dez;

    AX ? limit_coord_active(x, sbx, sex, dbx, dex) : limit_coord_passive(x, sbx, sex, dbx, dex);
    AY ? limit_coord_active(y, sby, sey, dby, dey) : limit_coord_passive(y, sby, sey, dby, dey);
    AZ ? limit_coord_active(z, sbz, sez, dbz, dez) : limit_coord_passive(z, sbz, sez, dbz, dez);

    for (int sz = sbz, dz = dbz; sz < sez; ++sz, ++dz)
    {
        for (int sy = sby, dy = dby; sy < sey; ++sy, ++dy)
        {
            for (int sx = sbx, dx = dbx; sx < sex; ++sx, ++dx)
            {
                dst->values[coords_to_cube_index(dx, dy, dz)] = src->values[coords_to_cube_index(sx, sy, sz)];
            }
        }
    }
}

template <typename T>
void rotate_refill_impl(T *dst, T *src[27], int x, int y, int z)
{
    auto dir_of = [](int v) { return v < 0 ? 1 : (v > 0 ? -1 : 0); };

    int dir_x = dir_of(x);
    int dir_y = dir_of(y);
    int dir_z = dir_of(z);

    if (x == 0 && y == 0 && z == 0)
    {
        *dst = *src[coords_to_neighbor_index(0, 0, 0)];
        return;
    }

    rotate_internal(src[coords_to_neighbor_index(0, 0, 0)], dst, x, y, z);

    if (x != 0)
    {
        range_copy<1, 0, 0>(dst, src[coords_to_neighbor_index(dir_x, 0, 0)], x, y, z);
    }

    if (y != 0)
    {
        range_copy<0, 1, 0>(dst, src[coords_to_neighbor_index(0, dir_y, 0)], x, y, z);
    }

    if (z != 0)
    {
        range_copy<0, 0, 1>(dst, src[coords_to_neighbor_index(0, 0, dir_z)], x, y, z);
    }

    if (x != 0 && y != 0)
    {
        range_copy<1, 1, 0>(dst, src[coords_to_neighbor_index(dir_x, dir_y, 0)], x, y, z);
    }

    if (y != 0 && z != 0)
    {
        range_copy<0, 1, 1>(dst, src[coords_to_neighbor_index(0, dir_y, dir_z)], x, y, z);
    }

    if (x != 0 && z != 0)
    {
        range_copy<1, 0, 1>(dst, src[coords_to_neighbor_index(dir_x, 0, dir_z)], x, y, z);
    }

    if (x != 0 && y != 0 && z != 0)
    {
        range_copy<1, 1, 1>(dst, src[coords_to_neighbor_index(dir_x, dir_y, dir_z)], x, y, z);
    }
}

void rotate_refill(cube_888_mask *dst, cube_888_mask *src[27], int x, int y, int z)
{
    rotate_refill_impl(dst, src, x, y, z);
}

void rotate_refill(cube_888_f32 *dst, cube_888_f32 *src[27], int x, int y, int z)
{
    rotate_refill_impl(dst, src, x, y, z);
}
} // namespace dvdb
