#pragma once

#include <cmath>
#include <cstdint>

// Not thoroughly tested. If this breaks, just define an empty macro.
#ifndef _WIN32
#define dvdb_restrict __restrict__
#else
#define dvdb_restrict __restrict
#endif

namespace dvdb
{
template <typename T>
struct cube_888
{
    T values[512];
};

using cube_888_f32 = cube_888<float>;
using cube_888_i8 = cube_888<int8_t>;

struct diff_desc
{
    uint32_t src_leaf, dst_leaf;
    float min, max;

    bool no_diff() const
    {
        return std::isnan(min);
    }

    void make_no_diff()
    {
        min = max = NAN;
    }
};

struct diff_rotation
{
    int16_t x : 4;
    int16_t y : 4;
    int16_t z : 4;

    int n_post_copy()
    {
        int i = 0;

        if (x != 0)
        {
            ++i;
        }

        if (y != 0)
        {
            ++i;
        }

        if (z != 0)
        {
            ++i;
        }

        return i;
    }
};

struct diff_post_copy
{
    uint32_t src_leaf;
};

struct diff_dct_header
{
    uint16_t code_points;
};

struct diff_dct_value
{
    int8_t value;
};

struct diff_dct_zero_length
{
    uint8_t length;
};
} // namespace dvdb
