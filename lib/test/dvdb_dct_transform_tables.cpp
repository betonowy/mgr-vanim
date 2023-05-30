#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <dct_transform_tables.hpp>

class dct_initialized
{
public:
    dct_initialized()
    {
        dvdb::dct_transform_tables_init();
    }
};

TEST_CASE_METHOD(dct_initialized, "encode_f32_constant_component")
{
    dvdb::cube_888_f32 src, dst;

    static constexpr auto TEST_VALUE = 1;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = TEST_VALUE;
    }

    dvdb::dct_transform_encode(&src, &dst);

    for (int i = 0; i < std::size(dst.values); ++i)
    {
        if (i == 0)
        {
            REQUIRE_THAT(dst.values[i], Catch::Matchers::WithinAbsMatcher(TEST_VALUE, 1e-6));
        }
        else
        {
            REQUIRE_THAT(dst.values[i], Catch::Matchers::WithinAbsMatcher(0, 1e-6));
        }
    }
}

TEST_CASE_METHOD(dct_initialized, "decode_f32_constant_component")
{
    dvdb::cube_888_f32 src{}, dst{};

    src.values[0] = 1;

    dvdb::dct_transform_decode(&src, &dst);

    for (int i = 0; i < std::size(dst.values); ++i)
    {
        REQUIRE_THAT(dst.values[i], Catch::Matchers::WithinAbsMatcher(1, 1e-6));
    }
}

TEST_CASE_METHOD(dct_initialized, "encode_decode_f32_smooth_values")
{
    dvdb::cube_888_f32 src{}, dct{}, res{};

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = std::cos(x * 2 * 3.141593 / 16.f);
    }

    dvdb::dct_transform_encode(&src, &dct);
    dvdb::dct_transform_decode(&dct, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 1e-5));
    }
}

TEST_CASE_METHOD(dct_initialized, "encode_decode_f32_smooth_values_2")
{
    dvdb::cube_888_f32 src{}, dct{}, res{};

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = std::cos(x) * std::cos(y * 1.23) * std::cos(z * 1.11);
        src.values[i] *= src.values[i];
    }

    dvdb::dct_transform_encode(&src, &dct);
    dvdb::dct_transform_decode(&dct, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 1e-5));
    }
}

TEST_CASE_METHOD(dct_initialized, "encode_decode_f32_random_values")
{
    dvdb::cube_888_f32 src{}, dct{}, res{};

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = rand() / static_cast<float>(RAND_MAX);
    }

    dvdb::dct_transform_encode(&src, &dct);
    dvdb::dct_transform_decode(&dct, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 1e-5));
    }
}

TEST_CASE_METHOD(dct_initialized, "encode_decode_f32_smooth_values_optimized_max_128_values")
{
    dvdb::cube_888_f32 src{}, dct{}, res{};

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = std::cos(x) * std::cos(y * 1.23) * std::cos(z * 1.11);
        src.values[i] *= src.values[i];
    }

    dvdb::dct_transform_encode(&src, &dct);
    {
        int count = 0;

        for (int i = 0; i < std::size(dct.values); ++i)
        {
            if (dct.values[i] != 0.f)
            {
                ++count;
            }
        }

        REQUIRE(count > 128);
    }
    {
        dvdb::dct_optimize(&dct, 0.f, 128);

        int count = 0;

        for (int i = 0; i < std::size(dct.values); ++i)
        {
            if (dct.values[i] != 0.f)
            {
                ++count;
            }
        }

        REQUIRE(count == 128);
    }
    dvdb::dct_transform_decode(&dct, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 8e-2));
    }
}

TEST_CASE_METHOD(dct_initialized, "encode_decode_f32_smooth_values_optimized_max_128_values_and_mse")
{
    dvdb::cube_888_f32 src{}, dct{}, res{};

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = std::cos(x) * std::cos(y * 1.23) * std::cos(z * 1.11);
        src.values[i] *= src.values[i];
    }

    dvdb::dct_transform_encode(&src, &dct);
    {
        int count = 0;

        for (int i = 0; i < std::size(dct.values); ++i)
        {
            if (dct.values[i] != 0.f)
            {
                ++count;
            }
        }

        REQUIRE(count > 128);
    }
    {
        dvdb::dct_optimize(&dct, 0.01f, 128);

        int count = 0;

        for (int i = 0; i < std::size(dct.values); ++i)
        {
            if (dct.values[i] != 0.f)
            {
                ++count;
            }
        }

        REQUIRE((count < 128 && count > 0));
    }
    dvdb::dct_transform_decode(&dct, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 0.5f));
    }
}
