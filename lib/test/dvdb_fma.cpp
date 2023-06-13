#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <fma.hpp>
#include <statistics.hpp>

#include <cstdio>
#include <cstring>

TEST_CASE("fma_match_constant_only")
{
    dvdb::cube_888_f32 src, dst, src_bis;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = 6 + i % 13;
        dst.values[i] = 9 + i % 13;
    }

    float add, mul;

    dvdb::find_fused_multiply_add(&src, &dst, &add, &mul);
    dvdb::fused_multiply_add(&src, &src_bis, add, mul);

    float mse_before = dvdb::mean_squared_error(&src, &dst);
    float mse_after = dvdb::mean_squared_error(&src_bis, &dst);

    CHECK(mse_before > mse_after);

    CHECK_THAT(mse_after, Catch::Matchers::WithinAbsMatcher(0.0, 1e-6));
    CHECK_THAT(add, Catch::Matchers::WithinRelMatcher(3.0, 1e-3));
    CHECK_THAT(mul, Catch::Matchers::WithinRelMatcher(1.0, 1e-3));
}

TEST_CASE("fma_match_multiply_only")
{
    dvdb::cube_888_f32 src, dst, src_bis;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = 6 + i % 13;
        dst.values[i] = src.values[i] * -2;
    }

    float add, mul;

    dvdb::find_fused_multiply_add(&src, &dst, &add, &mul);
    dvdb::fused_multiply_add(&src, &src_bis, add, mul);

    float mse_before = dvdb::mean_squared_error(&src, &dst);
    float mse_after = dvdb::mean_squared_error(&src_bis, &dst);

    CHECK(mse_before > mse_after);

    CHECK_THAT(mse_after, Catch::Matchers::WithinAbsMatcher(0.0, 1e-6));
    CHECK_THAT(add, Catch::Matchers::WithinAbsMatcher(0.0, 1e-3));
    CHECK_THAT(mul, Catch::Matchers::WithinRelMatcher(-2.0, 1e-3));
}

TEST_CASE("fma_match_complex")
{
    dvdb::cube_888_f32 src, dst, src_bis;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = 6 + i % 13;
        dst.values[i] = src.values[i] * -2 - 3;
    }

    float add, mul;

    dvdb::find_fused_multiply_add(&src, &dst, &add, &mul);
    dvdb::fused_multiply_add(&src, &src_bis, add, mul);

    float mse_before = dvdb::mean_squared_error(&src, &dst);
    float mse_after = dvdb::mean_squared_error(&src_bis, &dst);

    CHECK(mse_before > mse_after);

    CHECK_THAT(mse_after, Catch::Matchers::WithinAbsMatcher(0.0, 1e-6));
    CHECK_THAT(add, Catch::Matchers::WithinRelMatcher(-3.0, 1e-3));
    CHECK_THAT(mul, Catch::Matchers::WithinRelMatcher(-2.0, 1e-3));
}

TEST_CASE("fma_match_semi_random")
{
    dvdb::cube_888_f32 src, dst, src_bis;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = 6 + static_cast<int>(i + rand() / static_cast<float>(RAND_MAX)) % 13;
        dst.values[i] = (src.values[i] + rand() / static_cast<float>(RAND_MAX)) * -2 - 3;
    }

    float add, mul;

    dvdb::find_fused_multiply_add(&src, &dst, &add, &mul);
    dvdb::fused_multiply_add(&src, &src_bis, add, mul);

    float mse_before = dvdb::mean_squared_error(&src, &dst);
    float mse_after = dvdb::mean_squared_error(&src_bis, &dst);

    CHECK(mse_before > mse_after);

    CHECK_THAT(mse_after, Catch::Matchers::WithinAbsMatcher(0.33, 1e-3));
    CHECK_THAT(add, Catch::Matchers::WithinRelMatcher(-3.776, 1e-3));
    CHECK_THAT(mul, Catch::Matchers::WithinRelMatcher(-2.018, 1e-3));
}
