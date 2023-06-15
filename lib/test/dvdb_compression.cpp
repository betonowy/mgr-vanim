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
    char compression_output[2048]{};
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

float smoother_f32(int x, int y, int z)
{
    return rand() / static_cast<float>(RAND_MAX) * 0.1f + sin(x * 0.3 + 0.2 * y + 0.3 * z) + cos(y * 0.5f + 0.3 * x) - cos(z * 0.6 + 0.2 * x);
}

float rand_f32(int x, int y, int z)
{
    return rand() / static_cast<float>(RAND_MAX);
}

template <typename F>
float test_ratio(uint8_t quantization, F generator)
{
    static constexpr int count = 100;
    static constexpr int buffer_max_size = count * sizeof(dvdb::cube_888_i8);

    std::vector<dvdb::cube_888_f32> src_data(count);
    std::vector<dvdb::cube_888_i8> der_data(count);
    std::unique_ptr<char> compressed_data(new char[buffer_max_size]);

    for (auto &cube : src_data)
    {
        for (int i = 0; i < std::size(cube.values); ++i)
        {
            int x = (i & 0b000000111) >> 0;
            int y = (i & 0b000111000) >> 3;
            int z = (i & 0b111000000) >> 6;

            cube.values[i] = generator(x, y, z);
            cube.values[i] *= cube.values[i];
        }
    }

    for (int i = 0; i < count; ++i)
    {
        dvdb::cube_888_f32 dct;
        float min, max;

        dvdb::dct_3d_encode(src_data.data() + i, &dct);
        dvdb::encode_derivative_to_i8(&dct, der_data.data() + i, &min, &max, quantization);
    }

    float csize = dvdb::compress_stream(reinterpret_cast<const char *>(der_data.data()), der_data.size() * sizeof(dvdb::cube_888_i8), compressed_data.get(), buffer_max_size);

    return der_data.size() * sizeof(dvdb::cube_888_i8) / csize;
}

#define PRINT_GEN(desc) std::printf("%s: %f\n", #desc, desc);

TEST_CASE_METHOD(dvdb_init, "dct_ratio")
{
    PRINT_GEN(test_ratio(0x7f, rand_f32));
    PRINT_GEN(test_ratio(0x3f, rand_f32));
    PRINT_GEN(test_ratio(0x1f, rand_f32));

    PRINT_GEN(test_ratio(0x7f, smoother_f32));
    PRINT_GEN(test_ratio(0x3f, smoother_f32));
    PRINT_GEN(test_ratio(0x1f, smoother_f32));
}
