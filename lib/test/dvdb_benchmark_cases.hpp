#pragma once

#define restrict __restrict__

#include <immintrin.h>

namespace cases
{
static constexpr auto INTERFACE_SIZE = 512;
static constexpr auto interface_size_sse = INTERFACE_SIZE / 4;
static constexpr auto interface_size_avx = INTERFACE_SIZE / 8;

void copy_for_loop_scl(const float *restrict input, float *restrict output, const float *restrict work_group);
void copy_for_loop_avx(const float *restrict input, float *restrict output, const float *restrict work_group);

void cube_value_offset_scl(const float *restrict input, float *restrict output, int x, int y, int z);
void cube_value_offset_avx(const float *restrict input, float *restrict output, int x, int y, int z);

void cube_diff_scl(const float *restrict a, const float *restrict b, float *restrict diff);
void cube_diff_avx(const float *restrict a, const float *restrict b, float *restrict diff);

void cube_diff_epi8_scl(const int8_t *restrict a, const int8_t *restrict b, int8_t *restrict diff);
void cube_diff_epi8_avx(const int8_t *restrict a, const int8_t *restrict b, int8_t *restrict diff);

float cube_mse_scl(const float *restrict a, const float *restrict b);
float cube_mse_avx(const float *restrict a, const float *restrict b);

void prepare_trig_lut();

float sin_scl(float);
void sin_avx(float *in, float *out);

float cos_scl(float);
void cos_avx(float *in, float *out);

void compute_cube_dct_scl(float *restrict output, int x, int y, int z);
void compute_cube_dct_avx(float *restrict output, int x, int y, int z);

void offset_fill_x_scl(const float *restrict input, float *output, int x, int y, int z);
void offset_fill_y_scl(const float *restrict input, float *output, int x, int y, int z);
void offset_fill_z_scl(const float *restrict input, float *output, int x, int y, int z);

void offset_fill_x_avx(const float *restrict input, float *output, int x, int y, int z);
void offset_fill_y_avx(const float *restrict input, float *output, int x, int y, int z);
void offset_fill_z_avx(const float *restrict input, float *output, int x, int y, int z);

// generate cube from dct description
} // namespace cases
