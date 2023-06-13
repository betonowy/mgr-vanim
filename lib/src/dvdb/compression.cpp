#include "compression.hpp"

#include <lz4hc.h>

#ifndef LZ4_HC_STATIC_LINKING_ONLY
#error LZ4 is not statically linked. It must be.
#endif

namespace dvdb
{
thread_local LZ4_streamHC_t lz4_state_hc{};

int compress_stream(const char *input, int input_length, char *output, int output_length)
{
    return LZ4_compress_HC_extStateHC_fastReset(&lz4_state_hc, input, output, input_length, output_length, LZ4HC_CLEVEL_MAX);
}

int decompress_stream(const char *input, int input_length, char *output, int output_length)
{
    return LZ4_decompress_safe(input, output, input_length, output_length);
}
} // namespace dvdb
