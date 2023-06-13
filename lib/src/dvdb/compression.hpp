#pragma once

namespace dvdb
{
int compress_stream(const char *input, int input_length, char *output, int output_length);
int decompress_stream(const char *input, int input_length, char *output, int output_length);
} // namespace dvdb
