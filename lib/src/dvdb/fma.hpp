#pragma once

#include "types.hpp"

namespace dvdb
{
void find_fused_multiply_add(const cube_888_f32 *src, const cube_888_f32 *dst, float *add, float *mul);
void fused_multiply_add(const cube_888_f32 *src, cube_888_f32 *dst, float add, float mul);
} // namespace dvdb
