#pragma once

#include "types.hpp"

namespace dvdb
{
void add(const cube_888_f32 *lhs, const cube_888_f32 *rhs, cube_888_f32 *dst);
void sub(const cube_888_f32 *lhs, const cube_888_f32 *rhs, cube_888_f32 *dst);
void mul(const cube_888_f32 *lhs, const cube_888_f32 *rhs, cube_888_f32 *dst);
void fma(const cube_888_f32 *src, cube_888_f32 *dst, float add, float mul);

void div(const cube_888_f32 *lhs, const cube_888_f32 *rhs, cube_888_f32 *dst);
void div_fast(const cube_888_f32 *lhs, const cube_888_f32 *rhs, cube_888_f32 *dst);

void rcp(const cube_888_f32 *src, cube_888_f32 *dst);
void rsqrt(const cube_888_f32 *src, cube_888_f32 *dst);
void sqrt(const cube_888_f32 *src, cube_888_f32 *dst);
} // namespace dvdb
