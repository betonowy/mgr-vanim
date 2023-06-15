#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "common.hpp"
#include "compression.hpp"
#include "dct.hpp"
#include "derivative.hpp"
#include "rotate.hpp"
#include "statistics.hpp"

#include <scope_guard.hpp>

#include <chrono>
#include <cstring>
#include <glm/glm.hpp>
#include <immintrin.h>
#include <iostream>

namespace
{
static constexpr int test_time_ms = 100;
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

TEST_CASE("compression_64KiB_speed")
{
    static constexpr int input_length = 64 * 1024;

    char *input = static_cast<char *>(std::malloc(input_length));
    char *output = static_cast<char *>(std::malloc(input_length));

    utils::scope_guard defer([&] { std::free(input), std::free(output); });

    for (int i = 0; i < input_length; ++i)
    {
        input[i] = rand() % 0x1f;
    }

    int output_length;

    output_length = dvdb::compress_stream(input, input_length, output, input_length);

    BUDGET_BENCHMARK(dvdb::compress_stream, input, input_length, output, input_length);
    BUDGET_BENCHMARK(dvdb::decompress_stream, output, output_length, input, input_length);
}

TEST_CASE("derivative_speed")
{
    dvdb::cube_888_f32 src_f32{}, dst_f32{};
    dvdb::cube_888_i8 dst_i8;
    float min, max;

    BUDGET_BENCHMARK(dvdb::encode_derivative, &src_f32, &dst_f32);
    BUDGET_BENCHMARK(dvdb::decode_derivative, &src_f32, &dst_f32);

    BUDGET_BENCHMARK(dvdb::encode_derivative_to_i8, &src_f32, &dst_i8, &min, &max, 0x7f);
    BUDGET_BENCHMARK(dvdb::decode_derivative_from_i8, &dst_i8, &dst_f32, min, max, 0x7f);
}

TEST_CASE("statistics_speed")
{
    dvdb::cube_888_f32 a, b, mask;
    float x0, x1;

    dvdb::mean_squared_error(&a, &b);

    BUDGET_BENCHMARK(dvdb::mean, &a);
    BUDGET_BENCHMARK(dvdb::mean_squared_error, &a, &b);
    BUDGET_BENCHMARK(dvdb::mean_squared_error_with_mask, &a, &b, &mask);
    BUDGET_BENCHMARK(dvdb::accumulate, &a);
    BUDGET_BENCHMARK(dvdb::linear_regression, &a, &b, &x0, &x1);
}

static constexpr int coord_to_neighbor_index(int x, int y, int z)
{
    return (x + 1) + (y + 1) * 3 + (z + 1) * 3 * 3;
}

TEST_CASE("rotate_refill_speed")
{
    static constexpr glm::vec3 high_coord{24.f, 25.f, 26.f};
    dvdb::cube_888_f32 cubes[27], dst;
    int x, y, z;

    for (int i = -1; i <= 1; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            for (int k = -1; k <= 1; ++k)
            {
                const auto index = coord_to_neighbor_index(i, j, k);

                for (int x = 0; x < 8; ++x)
                {
                    for (int y = 0; y < 8; ++y)
                    {
                        for (int z = 0; z < 8; ++z)
                        {
                            const auto cube_index = x + (y << 3) + (z << 6);

                            glm::vec3 vcoord = (glm::vec3(i, j, k) + 1.f) * 8.f + glm::vec3(x, y, z);

                            cubes[index].values[cube_index] = glm::length(vcoord) / glm::length(high_coord);
                        }
                    }
                }
            }
        }
    }

    BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{ 5,  6,  7}", &dst, cubes, 5, 6, 7);
    BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{-3, -1, -2}", &dst, cubes, -3, -1, -2);
    BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{ 5,  6,  0}", &dst, cubes, 5, 6, 0);
    BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{-3, -1,  0}", &dst, cubes, -3, -1, 0);
    BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{ 5,  0,  0}", &dst, cubes, 5, 0, 0);
    BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{-3,  0,  0}", &dst, cubes, -3, 0, 0);
    BUDGET_BENCHMARK_CASE(dvdb::rotate_refill, "{ 0,  0,  0}", &dst, cubes, 0, 0, 0);

    static constexpr int ex = 2, ey = 1, ez = 3;

    dvdb::rotate_refill(&dst, cubes, ex, ey, ez);

    BUDGET_BENCHMARK(dvdb::rotate_refill_find_brute_force, &dst, cubes, &x, &y, &z);
    CHECK((x == ex && y == ey && z == ez));
    BUDGET_BENCHMARK(dvdb::rotate_refill_find_astar, &dst, cubes, &x, &y, &z);
    CHECK((x == ex && y == ey && z == ez));
}

TEST_CASE("fused_multiply_add_speed")
{
    dvdb::cube_888_f32 src{}, dst{}, src_bis{};
    float add, mul;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = rand() / static_cast<float>(RAND_MAX);
        dst.values[i] = rand() / static_cast<float>(RAND_MAX);
    }

    BUDGET_BENCHMARK(dvdb::linear_regression, &src, &dst, &add, &mul);
    BUDGET_BENCHMARK(dvdb::fma, &src, &src_bis, add, mul);
}

class dvdb_ready
{
public:
    dvdb_ready()
    {
        dvdb::dct_init();
    }
};

TEST_CASE_METHOD(dvdb_ready, "dct_speed")
{
    dvdb::cube_888_f32 src{}, dct{}, res{};

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = rand() / static_cast<float>(RAND_MAX);
    }

    BUDGET_BENCHMARK(dvdb::dct_3d_encode, &src, &dct);
    BUDGET_BENCHMARK(dvdb::dct_3d_decode, &dct, &res);
    BUDGET_BENCHMARK(dvdb::dct_3d_optimize, &dct, 0.005, 256);
    BUDGET_BENCHMARK_CASE(dvdb::dct_3d_decode, "{post optimize}", &dct, &res);
}
