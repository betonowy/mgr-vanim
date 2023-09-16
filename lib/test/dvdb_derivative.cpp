#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <derivative.hpp>
#include <statistics.hpp>

#include <iostream>

TEST_CASE("derivative_linear")
{
    dvdb::cube_888_f32 src, der, res;
    static constexpr auto base = 1;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = i + base;
    }

    dvdb::encode_derivative(&src, &der);

    for (int i = 0; i < std::size(der.values); ++i)
    {
        REQUIRE(der.values[i] == 1);
    }

    dvdb::decode_derivative(&der, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE(src.values[i] == res.values[i]);
    }
}

TEST_CASE("derivative_random")
{
    dvdb::cube_888_f32 src, der, res;
    static constexpr auto base = 1;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = rand() & (0xfff);
    }

    dvdb::encode_derivative(&src, &der);
    dvdb::decode_derivative(&der, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE(src.values[i] == res.values[i]);
    }
}

TEST_CASE("derivative_linear_i8")
{
    dvdb::cube_888_f32 src, res;
    dvdb::cube_888_i8 der;
    float min, max;

    static constexpr auto base = 1;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = (i + base) & 255;
    }

    dvdb::encode_derivative_to_i8(&src, &der, &min, &max, 0xff);
    dvdb::decode_derivative_from_i8(&der, &res, min, max, 0xff);

    auto error = dvdb::mean_squared_error(&src, &res);

    std::cout << "derivative_linear_i8 error: " << error << '\n';

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE(src.values[i] == res.values[i]);
    }
}

TEST_CASE("derivative_random_i8")
{
    dvdb::cube_888_f32 src, res;
    dvdb::cube_888_i8 der;
    float min, max;
    static constexpr auto base = 1;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = rand() / static_cast<float>(RAND_MAX);
    }

    dvdb::encode_derivative_to_i8(&src, &der, &min, &max, 0xff);
    dvdb::decode_derivative_from_i8(&der, &res, min, max, 0xff);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 0.5f / 255.f));
    }
}
