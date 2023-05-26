#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "dvdb_benchmark_cases.hpp"

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

#define BUDGET_BENCHMARK(func, ...) budget_benchmark(#func, [&]() { \
    func(__VA_ARGS__);                                              \
})
} // namespace

TEST_CASE("artificial_mul_copy_speed")
{
    static constexpr auto input_value = 1.f;
    static constexpr auto work_value = 2.f;
    static constexpr auto expected_value = input_value * work_value;

    alignas(alignof(__m256)) float input[cases::INTERFACE_SIZE] = {};
    alignas(alignof(__m256)) float output[cases::INTERFACE_SIZE] = {};
    alignas(alignof(__m256)) float work_group[cases::INTERFACE_SIZE] = {};

    for (int i = 0; i < cases::INTERFACE_SIZE; ++i)
    {
        input[i] = input_value;
        work_group[i] = work_value;
    }

    BUDGET_BENCHMARK(cases::copy_for_loop_scl, input, output, work_group);
    BUDGET_BENCHMARK(cases::copy_for_loop_avx, input, output, work_group);
}

TEST_CASE("artificial_shift_speed")
{
    float input[cases::INTERFACE_SIZE] = {};
    float output_scl[cases::INTERFACE_SIZE] = {};
    float output_avx[cases::INTERFACE_SIZE] = {};

    for (int i = 0; i < cases::INTERFACE_SIZE; ++i)
    {
        input[i] = i;
    }

    BUDGET_BENCHMARK(cases::cube_value_offset_scl, input, output_scl, 5, 3, 6);
    BUDGET_BENCHMARK(cases::cube_value_offset_avx, input, output_avx, 5, 3, 6);

    REQUIRE(std::memcmp(output_scl, output_avx, sizeof(output_scl)) == 0);

    for (int z = 0; z < 8; ++z)
    {
        for (int y = 0; y < 8; ++y)
        {
            for (int x = 0; x < 8; ++x)
            {
                cases::cube_value_offset_scl(input, output_scl, x, y, z);
                cases::cube_value_offset_avx(input, output_avx, x, y, z);

                if (std::memcmp(output_scl, output_avx, sizeof(output_scl)) != 0)
                {
                    FAIL("Incorrect results for " + ("x == " + std::to_string(x) + ", y == " + std::to_string(y) + ", z == " + std::to_string(z)));
                }
            }
        }
    }
}

glm::vec3 random_vector(float a, float b)
{
    auto get = [a, b]() {
        return rand() / static_cast<float>(RAND_MAX) * (b - a) + a;
    };

    return {get(), get(), get()};
}

float value_generator(glm::vec3 c)
{
    c *= 0.5f;

    float v = 0.0f;

    v += glm::sin(glm::cos(c.x + 1.34f) * 3.f + c.z * 21.f) + 1.f;
    v *= (glm::cos(glm::sin(c.y - 2.2f + c.z * 43.f) * 3.f + v) + 2.f) / 5.f;
    v *= glm::sin(-glm::cos(c.z * 100.f + 3.3f + c.x * 0.2f - c.y * c.y) * 0.2f) + 1.f;

    return v;
}

struct cube8f32
{
    float values[cases::INTERFACE_SIZE];
};

struct cube8i8
{
    int8_t values[cases::INTERFACE_SIZE];
};

void populate_cube(cube8f32 &cube, glm::vec3 offset)
{
    for (int i = 0; i < cases::INTERFACE_SIZE; ++i)
    {
        int x = (i >> 0) & 7;
        int y = (i >> 3) & 7;
        int z = (i >> 6) & 7;

        cube.values[i] = value_generator(offset + glm::vec3(x, y, z));
    }
}

void populate_cube(cube8i8 &cube, glm::vec3 offset)
{
    for (int i = 0; i < cases::INTERFACE_SIZE; ++i)
    {
        int x = (i >> 0) & 7;
        int y = (i >> 3) & 7;
        int z = (i >> 6) & 7;

        cube.values[i] = static_cast<int8_t>(value_generator(offset + glm::vec3(x, y, z)) * 127 - 127);
    }
}

TEST_CASE("artificial_diff_speed")
{
    static constexpr auto N_SRC_CUBES = 10;

    cube8f32 src_cubes[N_SRC_CUBES];
    glm::vec3 src_positions[N_SRC_CUBES];
    cube8f32 dst_cube;

    cube8f32 out_cube_scl;
    cube8f32 out_cube_avx;

    for (int i = 0; i < N_SRC_CUBES; ++i)
    {
        src_positions[i] = random_vector(-3, 3);
        populate_cube(src_cubes[i], src_positions[i]);
    }

    populate_cube(dst_cube, {});

    cases::cube_diff_scl(dst_cube.values, src_cubes[0].values, out_cube_scl.values);
    cases::cube_diff_avx(dst_cube.values, src_cubes[0].values, out_cube_avx.values);

    REQUIRE(std::memcmp(out_cube_scl.values, out_cube_avx.values, sizeof(out_cube_scl)) == 0);

    BUDGET_BENCHMARK(cases::cube_diff_scl, dst_cube.values, src_cubes[0].values, out_cube_scl.values);
    BUDGET_BENCHMARK(cases::cube_diff_avx, dst_cube.values, src_cubes[0].values, out_cube_scl.values);
}

TEST_CASE("artificial_diff_speed_epi8")
{
    static constexpr auto N_SRC_CUBES = 10;

    cube8i8 src_cubes[N_SRC_CUBES];
    glm::vec3 src_positions[N_SRC_CUBES];
    cube8i8 dst_cube;

    cube8i8 out_cube_scl;
    cube8i8 out_cube_avx;

    for (int i = 0; i < N_SRC_CUBES; ++i)
    {
        src_positions[i] = random_vector(-3, 3);
        populate_cube(src_cubes[i], src_positions[i]);
    }

    populate_cube(dst_cube, {});

    cases::cube_diff_epi8_scl(dst_cube.values, src_cubes[0].values, out_cube_scl.values);
    cases::cube_diff_epi8_avx(dst_cube.values, src_cubes[0].values, out_cube_avx.values);

    REQUIRE(std::memcmp(out_cube_scl.values, out_cube_avx.values, sizeof(out_cube_scl)) == 0);

    BUDGET_BENCHMARK(cases::cube_diff_epi8_scl, dst_cube.values, src_cubes[0].values, out_cube_scl.values);
    BUDGET_BENCHMARK(cases::cube_diff_epi8_avx, dst_cube.values, src_cubes[0].values, out_cube_scl.values);
}

TEST_CASE("artificial_mse_speed")
{
    static constexpr auto N_SRC_CUBES = 10;

    cube8f32 src_cubes[N_SRC_CUBES];
    glm::vec3 src_positions[N_SRC_CUBES];
    cube8f32 dst_cube;

    for (int i = 0; i < N_SRC_CUBES; ++i)
    {
        src_positions[i] = random_vector(-3, 3);
        populate_cube(src_cubes[i], src_positions[i]);
    }

    populate_cube(dst_cube, {});

    float out_scl = cases::cube_mse_scl(dst_cube.values, src_cubes[0].values);
    float out_avx = cases::cube_mse_avx(dst_cube.values, src_cubes[0].values);

    REQUIRE_THAT(out_scl, Catch::Matchers::WithinRelMatcher(out_avx, 1e-6));

    BUDGET_BENCHMARK(cases::cube_mse_scl, dst_cube.values, src_cubes[0].values);
    BUDGET_BENCHMARK(cases::cube_mse_avx, dst_cube.values, src_cubes[0].values);
}

TEST_CASE("artificial_sin_speed")
{
    float values[4096];
    float reference[4096];
    static constexpr float multiplier = 0.001224252;

    cases::prepare_trig_lut();

    for (int i = 0; i < 4096; ++i)
    {
        float v = i * multiplier;

        values[i] = v;
        reference[i] = std::sin(v);
    }

    for (int i = 0; i < 4096; ++i)
    {
        float v = i * multiplier;

        float a = reference[i];
        float b = cases::sin_scl(v);

        REQUIRE_THAT(a, Catch::Matchers::WithinAbsMatcher(b, 1e-4));
    }

    // for (int i = 0; i < 4096; i += 8)
    // {
    //     float out[8];

    //     cases::sin_avx_t7(values + i, out);

    //     for (int j = 0; j < 8; ++j)
    //     {
    //         REQUIRE_THAT(reference[i + j], Catch::Matchers::WithinAbsMatcher(out[j], 1e-3));
    //     }
    // }

    // for (int i = 0; i < 4096; i += 8)
    // {
    //     float out[8];

    //     cases::sin_avx_t11(values + i, out);

    //     for (int j = 0; j < 8; ++j)
    //     {
    //         REQUIRE_THAT(reference[i + j], Catch::Matchers::WithinAbsMatcher(out[j], 1e-3));
    //     }
    // }
}

TEST_CASE("artificial_dct_speed")
{
    cube8f32 out_scl, out_avx;

    cases::compute_cube_dct_scl(out_scl.values, 1, 2, 3);
    cases::compute_cube_dct_avx(out_avx.values, 1, 2, 3);

    for (int i = 0; i < cases::INTERFACE_SIZE; ++i)
    {
        // REQUIRE_THAT(out_scl.values[i], Catch::Matchers::WithinAbsMatcher(out_avx.values[i], 1e-3));
    }

    // BUDGET_BENCHMARK(cases::compute_cube_dct_scl, out_scl.values, 1, 2, 3);
    // BUDGET_BENCHMARK(cases::compute_cube_dct_avx, out_avx.values, 1, 2, 3);
}
