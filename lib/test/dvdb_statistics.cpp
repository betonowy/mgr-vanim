#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <statistics.hpp>

TEST_CASE("accumulate")
{
    dvdb::cube_888_f32 src;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = i + 0.5f;
    }

    float acc = dvdb::accumulate(&src);

    CHECK_THAT(acc, Catch::Matchers::WithinRelMatcher(std::size(src.values) * std::size(src.values) / 2.f, 1e-12));
}

TEST_CASE("mean")
{
    dvdb::cube_888_f32 src;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = i + 0.5f;
    }

    float acc = dvdb::mean(&src);

    CHECK_THAT(acc, Catch::Matchers::WithinRelMatcher(std::size(src.values) / 2.f, 1e-12));
}

TEST_CASE("mean_squared_error")
{
    dvdb::cube_888_f32 src, dst;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = 4.f;
        dst.values[i] = 5.f;
    }

    float mse = dvdb::mean_squared_error(&src, &dst);

    CHECK_THAT(mse, Catch::Matchers::WithinAbsMatcher(1.f, 1e-2));
}

TEST_CASE("mean_squared_error_with_mask")
{
    dvdb::cube_888_f32 src, dst, mask;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = 4.f;
        dst.values[i] = 5.f;

        mask.values[i] = i < 256 ? 0.f : 1.f;
    }

    float mse = dvdb::mean_squared_error_with_mask(&src, &dst, &mask);

    CHECK_THAT(mse, Catch::Matchers::WithinAbsMatcher(.5f, 1e-2));
}

TEST_CASE("linear_regression")
{
    dvdb::cube_888_f32 src, dst;
    float add, mul;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = i;
        dst.values[i] = i;
    }

    dvdb::linear_regression(&src, &dst, &add, &mul);

    CHECK_THAT(add, Catch::Matchers::WithinAbsMatcher(0.f, 1e-6));
    CHECK_THAT(mul, Catch::Matchers::WithinAbsMatcher(1.f, 1e-6));

    // avoid dividing by zero
    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = 11 + i;
        dst.values[i] = 21 + i;
    }

    dvdb::linear_regression(&src, &dst, &add, &mul);

    CHECK_THAT(add, Catch::Matchers::WithinAbsMatcher(10.f, 1e-6));
    CHECK_THAT(mul, Catch::Matchers::WithinAbsMatcher(1.f, 1e-6));

    // avoid dividing by zero
    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = i;
        dst.values[i] = i * 10;
    }

    dvdb::linear_regression(&src, &dst, &add, &mul);

    CHECK_THAT(add, Catch::Matchers::WithinAbsMatcher(0.f, 2e-3));
    CHECK_THAT(mul, Catch::Matchers::WithinAbsMatcher(10.f, 1e-5));
}
