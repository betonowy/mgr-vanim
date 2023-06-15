#pragma once

#include "types.hpp"

namespace dvdb
{
float mean(const cube_888_f32 *src);
float mean_squared_error(const cube_888_f32 *a, const cube_888_f32 *b);
float mean_squared_error_with_mask(const cube_888_f32 *a, const cube_888_f32 *b, const cube_888_f32 *mask);

float accumulate(const cube_888_f32 *src);
void linear_regression(const cube_888_f32 *x, const cube_888_f32 *y, float *add, float *mul);
void linear_regression_with_mask(const cube_888_f32 *x, const cube_888_f32 *y, float *add, float *mul, const cube_888_f32 *mask);
} // namespace dvdb
