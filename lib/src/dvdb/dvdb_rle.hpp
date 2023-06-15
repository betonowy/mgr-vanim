#pragma once

#include <cassert>
#include <cstdint>
#include <numeric>

namespace dvdb
{
struct code_desc
{
    uint32_t src_leaf, dst_leaf;
    float min, max;
};

struct rle_diff_code
{
    // union {
    //     struct
    //     {
    //         uint16_t length, value;
    //     };

    //     uint32_t bits;
    // };

    // float float_value(const rle_diff_desc &desc)
    // {
    //     return desc.min + (desc.max - desc.min) * (value * (1.f / std::numeric_limits<decltype(value)>::max()));
    // }

    // void set_value(const rle_diff_desc &desc, float value)
    // {
    //     assert(value <= desc.max && value >= desc.min);
    // }

    float values[512];
};
} // namespace dvdb
