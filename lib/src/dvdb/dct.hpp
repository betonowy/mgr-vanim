#pragma once

#include "types.hpp"

namespace dvdb
{
template <typename T>
struct dct_tables_888
{
    T tables[512];
};

extern dct_tables_888<cube_888_f32> dct_3d_f32;
extern dct_tables_888<cube_888_i8> dct_3d_i8;

void dct_init();

// void dct_3d_encode_fdct(const cube_888_f32 *src, cube_888_f32 *dst); // too much time required for now (5.06.2023)
void dct_3d_encode(const cube_888_f32 *src, cube_888_f32 *dst);
void dct_3d_decode(const cube_888_f32 *src, cube_888_f32 *dst);
void dct_accumulate_decode_cell(float wgt, const cube_888_f32 *dct, cube_888_f32 *dst);
void dct_3d_optimize(cube_888_f32 *dct, float max_mse, int max_tables);

// FIXME nothing to gain from these additional steps. Maybe delete.
void dct_1d_encode(const cube_888_f32 *src, cube_888_f32 *dst);
void dct_1d_decode(const cube_888_f32 *src, cube_888_f32 *dst);
void dct_1d_accumulate_decode_cell(float wgt, const cube_888_f32 *dct, cube_888_f32 *dst);
void dct_1d_optimize(cube_888_f32 *dct, float max_mse, int max_tables);
} // namespace dvdb
