#include "derivative.hpp"

#include "immintrin.h"

#include <iterator>

namespace dvdb
{
void encode_derivative(const cube_888_f32 *src, cube_888_f32 *der)
{
    float prev_value = 0;

    for (int i = 0; i < std::size(der->values); ++i)
    {
        float new_value = static_cast<float>(static_cast<int>(std::round(src->values[i] * 32))) / 32;
        der->values[i] = new_value - prev_value;
        prev_value = new_value;
    }
}

void decode_derivative(const cube_888_f32 *der, cube_888_f32 *res)
{
    float prev_value = 0;

    for (int i = 0; i < std::size(der->values); ++i)
    {
        prev_value += der->values[i];
        res->values[i] = prev_value;
    }
}

void map_values(float src_a, float src_b, float dst_a, float dst_b, const cube_888_f32 *src, cube_888_f32 *dst)
{
    float ratio = (dst_b - dst_a) / (src_b - src_a);

    for (int i = 0; i < std::size(dst->values); i += 1)
    {
        dst->values[i] = (src->values[i] - src_a) * ratio + dst_a;
    }
}

static constexpr auto range_a = 0;

void encode_derivative_to_i8(const cube_888_f32 *src, cube_888_i8 *der, float *max, float *min, uint8_t quantization_limit)
{
    *min = *max = src->values[0];

    for (int i = 1; i < std::size(src->values); ++i)
    {
        *max = std::max(*max, src->values[i]);
        *min = std::min(*min, src->values[i]);
    }

    cube_888_f32 mid;
    map_values(*min, *max, range_a, quantization_limit, src, &mid);

    uint8_t prev_value = 0;

    for (int i = 0; i < std::size(der->values); ++i)
    {
        uint8_t new_value = std::round(mid.values[i]);
        der->values[i] = new_value - prev_value;
        prev_value = new_value;
    }
}

void decode_derivative_from_i8(const cube_888_i8 *der, cube_888_f32 *res, float max, float min, uint8_t quantization_limit)
{
    cube_888_f32 mid;

    uint8_t prev_value = 0;

    for (int i = 0; i < std::size(der->values); ++i)
    {
        prev_value += der->values[i];
        mid.values[i] = prev_value;
    }

    map_values(range_a, quantization_limit, min, max, &mid, res);
}
} // namespace dvdb
