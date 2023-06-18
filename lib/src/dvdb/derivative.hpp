#pragma once

#include "types.hpp"

namespace dvdb
{
void encode_derivative(const cube_888_f32 *, cube_888_f32 *);
void decode_derivative(const cube_888_f32 *, cube_888_f32 *);

void encode_derivative_to_i8(const cube_888_f32 *, cube_888_i8 *, float *max, float *min, uint8_t quantization_limit);
void decode_derivative_from_i8(const cube_888_i8 *, cube_888_f32 *, float max, float min, uint8_t quantization_limit);

void encode_to_i8(const cube_888_f32 *, cube_888_i8 *, float *max, float *min, uint8_t quantization_limit);
void decode_from_i8(const cube_888_i8 *, cube_888_f32 *, float max, float min, uint8_t quantization_limit);
} // namespace dvdb
