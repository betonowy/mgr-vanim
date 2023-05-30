#pragma once

#include "types.hpp"

namespace dvdb
{
template <typename T>
struct dct_tables_888
{
    T tables[512];
};

extern dct_tables_888<cube_888_f32> dct_f32;
extern dct_tables_888<cube_888_i8> dct_i8;

void dct_transform_tables_init();

void dct_transform_encode(const cube_888_f32 *src, cube_888_f32 *dst);
void dct_transform_decode(const cube_888_f32 *src, cube_888_f32 *dst);
void dct_transform_accumulate_decode_cell(float wgt, const cube_888_f32 *dct, cube_888_f32 *dst);
void dct_optimize(cube_888_f32 *dct, float max_mse, int max_tables);
} // namespace dvdb
