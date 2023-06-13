#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "compression.hpp"
#include "dct.hpp"
#include "derivative.hpp"
#include "fma.hpp"
#include "statistics.hpp"
#include "rotate.hpp"

#include <scope_guard.hpp>

#include <chrono>
#include <cstring>
#include <glm/glm.hpp>
#include <immintrin.h>
#include <iostream>

namespace
{
static constexpr int test_time_ms = 1000;
static constexpr int frame_time_ms = 33;

template <typename F>
float budget_benchmark(const char *description, F f)
{
    auto tbegin = std::chrono::steady_clock::now();
    auto tend_cnd = tbegin + std::chrono::milliseconds(test_time_ms);
    std::chrono::steady_clock::time_point tend_actual;

    size_t count = 0;

    while ((tend_actual = std::chrono::steady_clock::now()) < tend_cnd)
    {
        static constexpr auto granularity = 128;

        for (int i = 0; i < granularity; ++i)
        {
            f();
        }

        count += granularity;
    }

    float time_ms = std::chrono::duration_cast<std::chrono::nanoseconds>(tend_actual - tbegin).count() * 1e-6;

    std::cout << description << ": executed " << static_cast<size_t>(count * frame_time_ms / time_ms) << " times per frame (33 ms). Single iteration: " << 1e3 * time_ms / count << " us.\n";

    return count;
}

#define BUDGET_BENCHMARK_CASE(func, note, ...) budget_benchmark(#func "" note, [&]() { \
    func(__VA_ARGS__);                                                                 \
})

#define BUDGET_BENCHMARK(func, ...) budget_benchmark(#func, [&]() { \
    func(__VA_ARGS__);                                              \
})
} // namespace

// TEST_CASE("compression_64KiB_speed")
// {
//     static constexpr int input_length = 64 * 1024;

//     char *input = static_cast<char *>(std::malloc(input_length));
//     char *output = static_cast<char *>(std::malloc(input_length));

//     utils::scope_guard defer([&] { std::free(input), std::free(output); });

//     for (int i = 0; i < input_length; ++i)
//     {
//         input[i] = rand() % 0x1f;
//     }

//     int output_length;

//     output_length = dvdb::compress_stream(input, input_length, output, input_length);

//     BUDGET_BENCHMARK(dvdb::compress_stream, input, input_length, output, input_length);
//     BUDGET_BENCHMARK(dvdb::decompress_stream, output, output_length, input, input_length);
// }

// TEST_CASE("derivative_speed")
// {
//     dvdb::cube_888_f32 src_f32{}, dst_f32{};
//     dvdb::cube_888_i8 dst_i8;
//     float min, max;

//     BUDGET_BENCHMARK(dvdb::encode_derivative, &src_f32, &dst_f32);
//     BUDGET_BENCHMARK(dvdb::decode_derivative, &src_f32, &dst_f32);

//     BUDGET_BENCHMARK(dvdb::encode_derivative_to_i8, &src_f32, &dst_i8, &min, &max, 0x7f);
//     BUDGET_BENCHMARK(dvdb::decode_derivative_from_i8, &dst_i8, &dst_f32, min, max, 0x7f);
// }

// TEST_CASE("mean_squared_error_speed")
// {
//     dvdb::cube_888_f32 a, b, mask;

//     dvdb::mean_squared_error(&a, &b);

//     BUDGET_BENCHMARK(dvdb::mean_squared_error, &a, &b);
//     BUDGET_BENCHMARK(dvdb::mean_squared_error_with_mask, &a, &b, &mask);
// }

// TEST_CASE("rotate_refill_speed")
// {
//     dvdb::cube_888_f32 cubes[27], dst;

//     BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{ 5,  6,  7}", &dst, cubes, 5, 6, 7);
//     BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{-3, -1, -2}", &dst, cubes, -3, -1, -2);
//     BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{ 5,  6,  0}", &dst, cubes, 5, 6, 0);
//     BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{-3, -1,  0}", &dst, cubes, -3, -1, 0);
//     BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{ 5,  0,  0}", &dst, cubes, 5, 0, 0);
//     BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{-3,  0,  0}", &dst, cubes, -3, 0, 0);
//     BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{ 0,  0,  0}", &dst, cubes, 0, 0, 0);
// }

TEST_CASE("fused_multiply_add_speed")
{
    dvdb::cube_888_f32 src{}, dst{}, src_bis{};
    float add, mul;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = rand() / static_cast<float>(RAND_MAX);
        dst.values[i] = rand() / static_cast<float>(RAND_MAX);
    }

    BUDGET_BENCHMARK(dvdb::find_fused_multiply_add, &src, &dst, &add, &mul);
    BUDGET_BENCHMARK(dvdb::fused_multiply_add, &src, &src_bis, add, mul);
}

class dvdb_ready
{
public:
    dvdb_ready()
    {
        dvdb::dct_init();
    }
};

// TEST_CASE_METHOD(dvdb_ready, "dct_speed")
// {
//     dvdb::cube_888_f32 src{}, dct{}, res{};

//     for (int i = 0; i < std::size(src.values); ++i)
//     {
//         int x = (i & 0b000000111) >> 0;
//         int y = (i & 0b000111000) >> 3;
//         int z = (i & 0b111000000) >> 6;

//         src.values[i] = rand() / static_cast<float>(RAND_MAX);
//     }

//     BUDGET_BENCHMARK(dvdb::dct_3d_encode, &src, &dct);
//     BUDGET_BENCHMARK(dvdb::dct_3d_decode, &dct, &res);
// }
