#pragma once

#ifndef _WIN32
#define dvdb_restrict __restrict__
#else
#define dvdb_restrict __restrict
#endif

#include <immintrin.h>

#include <cstdint>

namespace cases
{
static constexpr auto INTERFACE_SIZE = 512;
static constexpr auto interface_size_sse = INTERFACE_SIZE / 4;
static constexpr auto interface_size_avx = INTERFACE_SIZE / 8;

void copy_for_loop_scl(const float *dvdb_restrict input, float *dvdb_restrict output, const float *dvdb_restrict work_group);
void copy_for_loop_avx(const float *dvdb_restrict input, float *dvdb_restrict output, const float *dvdb_restrict work_group);

void cube_value_offset_scl(const float *dvdb_restrict input, float *dvdb_restrict output, int x, int y, int z);
void cube_value_offset_avx(const float *dvdb_restrict input, float *dvdb_restrict output, int x, int y, int z);

void cube_diff_scl(const float *dvdb_restrict a, const float *dvdb_restrict b, float *dvdb_restrict diff);
void cube_diff_avx(const float *dvdb_restrict a, const float *dvdb_restrict b, float *dvdb_restrict diff);

void cube_diff_epi8_scl(const int8_t *dvdb_restrict a, const int8_t *dvdb_restrict b, int8_t *dvdb_restrict diff);
void cube_diff_epi8_avx(const int8_t *dvdb_restrict a, const int8_t *dvdb_restrict b, int8_t *dvdb_restrict diff);

float cube_mse_scl(const float *dvdb_restrict a, const float *dvdb_restrict b);
float cube_mse_avx(const float *dvdb_restrict a, const float *dvdb_restrict b);

void prepare_trig_lut();

float sin_scl(float);
void sin_avx(float *in, float *out);

float cos_scl(float);
void cos_avx(float *in, float *out);

void compute_cube_dct_scl(float *dvdb_restrict output, int x, int y, int z);
void compute_cube_dct_avx(float *dvdb_restrict output, int x, int y, int z);

void offset_fill_x_scl(const float *dvdb_restrict input, float *output, int x, int y, int z);
void offset_fill_y_scl(const float *dvdb_restrict input, float *output, int x, int y, int z);
void offset_fill_z_scl(const float *dvdb_restrict input, float *output, int x, int y, int z);

void offset_fill_x_avx(const float *dvdb_restrict input, float *output, int x, int y, int z);
void offset_fill_y_avx(const float *dvdb_restrict input, float *output, int x, int y, int z);
void offset_fill_z_avx(const float *dvdb_restrict input, float *output, int x, int y, int z);

// generate cube from dct description
} // namespace cases
