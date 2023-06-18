#pragma once

#include "types.hpp"

namespace dvdb
{
extern cube_888<uint16_t> hilbert_curve_indices;

// FIXME it is likely that these won't be used and only coord_transform_indices will be
// used and only in dct table initialization and nowhere else... maybe delete it later.
void transform_carthesian_to_hilbert(const cube_888_f32 *src, cube_888_f32 *dst);
void transform_carthesian_to_hilbert(const cube_888_i8 *src, cube_888_i8 *dst);
void transform_hilbert_to_carthesian(const cube_888_f32 *src, cube_888_f32 *dst);
void transform_hilbert_to_carthesian(const cube_888_i8 *src, cube_888_i8 *dst);
} // namespace dvdb
