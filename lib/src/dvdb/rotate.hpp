#pragma once

#include "types.hpp"

namespace dvdb
{
float rotate_refill_find(const cube_888_f32 *dst, const cube_888_f32 src[27], int *x, int *y, int *z);
void rotate_refill(cube_888_f32 *dst, const cube_888_f32 src[27], int x, int y, int z);
} // namespace dvdb
