#include "coord_transform.hpp"

#include <algorithm>
#include <iterator>

namespace dvdb
{
cube_888<uint16_t> coord_transform_indices;

namespace
{
template <typename T>
void carthesian_to_zigzag_impl(const cube_888<T> *src, cube_888<T> *dst)
{
    for (int i = 0; i < std::size(coord_transform_indices.values); ++i)
    {
        dst->values[i] = src->values[coord_transform_indices.values[i]];
    }
}

template <typename T>
void zigzag_to_carthesian_impl(const cube_888<T> *src, cube_888<T> *dst)
{
    for (int i = 0; i < std::size(coord_transform_indices.values); ++i)
    {
        dst->values[coord_transform_indices.values[i]] = src->values[i];
    }
}

} // namespace
void coord_transform_carthesian_to_zigzag(const cube_888_f32 *src, cube_888_f32 *dst)
{
    carthesian_to_zigzag_impl(src, dst);
}

void coord_transform_carthesian_to_zigzag(const cube_888_i8 *src, cube_888_i8 *dst)
{
    carthesian_to_zigzag_impl(src, dst);
}

void coord_transform_zigzag_to_carthesian(const cube_888_f32 *src, cube_888_f32 *dst)
{
    zigzag_to_carthesian_impl(src, dst);
}

void coord_transform_zigzag_to_carthesian(const cube_888_i8 *src, cube_888_i8 *dst)
{
    zigzag_to_carthesian_impl(src, dst);
}

void coord_transform_init()
{
    for (int i = 0; i < std::size(coord_transform_indices.values); ++i)
    {
        coord_transform_indices.values[i] = i;
    }

    std::sort(std::begin(coord_transform_indices.values), std::end(coord_transform_indices.values), [](uint16_t lhs, uint16_t rhs) -> bool {
        int lhs_x = (lhs & 0b000000111) >> 0;
        int lhs_y = (lhs & 0b000111000) >> 3;
        int lhs_z = (lhs & 0b111000000) >> 6;

        int rhs_x = (rhs & 0b000000111) >> 0;
        int rhs_y = (rhs & 0b000111000) >> 3;
        int rhs_z = (rhs & 0b111000000) >> 6;

        int lhs_sum = lhs_x * lhs_x + lhs_y * lhs_y + lhs_z * lhs_z;
        int rhs_sum = rhs_x * rhs_x + rhs_y * rhs_y + rhs_z * rhs_z;

        return (lhs_sum < rhs_sum) || (lhs < rhs);
    });
}
} // namespace dvdb
