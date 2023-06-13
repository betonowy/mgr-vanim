#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <nanovdb/PNanoVDB.h>

#include <compression.hpp>
#include <dct.hpp>
#include <derivative.hpp>
#include <nvdb_mmap.hpp>
#include <resource_path.hpp>

class dvdb_init
{
public:
    dvdb_init()
    {
        dvdb::dct_init();

        dvdb::cube_888_f32 src{}, dct{};

        for (int i = 0; i < std::size(src.values); ++i)
        {
            int x = (i & 0b000000111) >> 0;
            int y = (i & 0b000111000) >> 3;
            int z = (i & 0b111000000) >> 6;

            src.values[i] = std::cos(x * 0.33 + y * 0.51 + z * 0.13 + 3) * std::cos(y * 0.63 + x * 0.37 + z * 0.44 + 0.3) + std::cos(z * 0.461 + x * 0.1 + y * 0.22 + 2);
            src.values[i] *= src.values[i];
        }

        float min, max;
        static constexpr uint8_t quantization_limit = 0x1f;

        dvdb::dct_3d_encode(&src, &dct);
        dvdb::encode_derivative_to_i8(&dct, &dctderi8, &min, &max, quantization_limit);

        dctderi8_ptr = reinterpret_cast<char *>(&dctderi8);
    }

    dvdb::cube_888_i8 dctderi8;

    char *dctderi8_ptr;
};

TEST_CASE_METHOD(dvdb_init, "compress_and_decompress_stream")
{
    char compression_output[2048];
    dvdb::cube_888_i8 decompression_output;

    int compressed = dvdb::compress_stream(dctderi8_ptr, sizeof(dctderi8), compression_output, sizeof(compression_output));
    REQUIRE((compressed < sizeof(dctderi8) / 2 && compressed > 0));

    int decompressed = dvdb::decompress_stream(compression_output, compressed, reinterpret_cast<char *>(&decompression_output), sizeof(decompression_output));
    REQUIRE(decompressed == sizeof(dctderi8));

    for (int i = 0; i < std::size(dctderi8.values); ++i)
    {
        REQUIRE(decompression_output.values[i] == dctderi8.values[i]);
    }
}
